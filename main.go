package main

import (
	"runtime"
	"flag"
	"os"
	"os/signal"
	"syscall"
	"fmt"
	"net"
	"bufio"
	"io"
	"sync"
	clipboard "clipboard/clip"
	"av-forward/server"
	"clipboard/protocol"

)

//Have function that gets current top of clipboard. 
//Send if it is separate from the last thing sent.
//Lock everything. Run send and recieve in different threads
//Use gob to serialize
//Test sending using netcat

var currentSelection protocol.CipboardData
var currentSelectionLock sync.RWMutex

var itemsRecieved=make([]protocol.ClipboardData,0)
var itemsRecievedLock sync.RWMutex

func getCurrentSelection(){
	//Get HasChanged working
	for {
		currentSelectionLock.Lock()
		currentSelection=clipboard.Get();
		currentSelectionLock.Unlock();
	}

}

func readFromRemote(conn io.Reader){
	scanner:=bufio.NewScanner(conn)

	for scanner.Scan(){
		itemsRecievedLock.Lock()
		itemsRecieved=append(itemsRecieved,protocol.Decode(scanner.Bytes()))
		itemsRecievedLock.Unlock()
	}
}
func Send(conn io.Writer){
	var lastItemSent struct{}
	for {
		currentSelectionLock.RLock()
		if currentSelection!=lastItemSent{
			itemToSend:=protocol.Encode(currentSelection)
			lastItemSent=currentSelection
			currentSelection.RUnlock()
			conn.Write(itemToSend) //Encode with new line
		}else{
			currentSelectionLock.RUnlock()
		}
	}
}

func Receive(conn io.Reader){
	//This syncing works if we assume any item sent is processed instantaneously by all clients. If there is a non-trivial delay (which means the clipboard on the server has changed), there is no way to know whether the sent data is from the server (in that case, we should not send it again), or without doing some kind of signaling

	var item struct{}

	go readFromRemote(conn)

	for {
		itemsRecievedLock.Lock()
		if len(itemsRecieved)>0{
			item=itemsRecieved[0]
			itemsRecieved=itemsRecieved[1:]
		}
		itemsRecievedLock.Unlock()

		currentSelection.RLock()
		if currentSelection!=item{
			clipboard.Set(item)
		}
		currentSelection.RUnlock()
	}

}

func exitHandler(){
	sigs:= make(chan os.Signal, 1)
	signal.Notify(sigs, os.Interrupt, syscall.SIGTERM)
	<-sigs
	os.Exit(0)
}


func main(){
	go exitHandler()

	var host bool
	var host_help_text string
	var conn io.ReadWriter

	if runtime.GOOS=="darwin"{
		host=true
	}else if runtime.GOOS=="linux"{
		host=false
	}

	if host{
		host_help_text="server will be running on"
	}else{
		host_help_text="client will be connecting to"
	}

	Host:=flag.String("host","127.0.0.1",fmt.Sprintf("IP where the %s",host_help_text))
	flag.Parse()

	if host{
		server:=new(server.Server)
		server.Start(*Host,8001)
		conn=server
	}else{
		conn,_=net.Dial("tcp",fmt.Sprintf("%s:%d",*Host,8001))
	}

	go getCurrentSelection()
	go Send(conn)
	go Receive(conn)

	select{} //Sleep forever


}