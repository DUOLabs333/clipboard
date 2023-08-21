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

var clipboardItems=make([]protocol.Selection,0)
var clipboardItemsLock sync.RWMuxtex

var clipboardSources=make([]string,0)
var clipboardSourcesLock sync.RWMuxtex

var recievedItems=map[string]int
var recievedItemsLock sync.RWMuxtex


func lockItems(){
	clipboardItemsLock.Lock()
	clipboardSourcesLock.Lock()
}

func unlockItems(){
		clipboardItemsLock.UnLock();
	clipboardSourcesLock.Unlock()
}

func rLockItems(){
	clipboardItemsLock.RLock()
	clipboardSourcesLock.RLock()
}

func rUnlockItems(){
		clipboardItemsLock.RUnLock();
	clipboardSourcesLock.RUnlock()
}

func queueItems(src string,selection protocol.Selection){
	lockItems()
	defer unlockItems()

	clipboardItems=append(clipboardItems,selection);
	clipboardSources=append(clipboardSources,src);


}

func dequeueItems() (src string,selection protocol.Selection){
	lockItems()
	defer unlockItems()

	src=clipboardSources[0]
	selection=clipboardItems[0]

	clipboardSources=clipboardSources[1:]
	clipboardItems=clipboardItems[1:]

	clipboardItemsLock.UnLock();
	clipboardSourcesLock.Unlock()
}


func readFromLocal(){
	for {
		if clipboard.clipboardHasChanged(){
			selection:=clipboard.Get();

			//Don't want to send the same thing twice --- may not be needed with clipboardHasChanged
			rLockItems()
			if selection==clipboardItems[-1]{
				rUnlockItems()
				return
			}
			rUnlockItems()

			hash := protocol.Hash(selection)

			//Proto-ACK functionality. If you recieve selection, don't send the same selection (will just lead to a loop). Acknowledge you recieved it by ignoring it in the clipboard.

			recievedItemsLock.Lock()
			if recievedItems[hash]>0{
				recievedItems[hash]-=1
				return
			}
			recievedItemsLock.Unlock()

			queueItems("local",selection);
			
		}
	}

}


func readFromRemote(conn io.Reader){
	scanner:=bufio.NewScanner(conn)

	for scanner.Scan(){
		selection:=protocol.Decode(scanner.Bytes())

		hash := protocol.Hash(selection)

		recievedItemsLock.Lock()
		recievedItems[hash]+=1
		recievedItemsLock.Unlock()

		queueItems("remote",selection)
	}
}

func Process(conn io.Writer){
	for{
		//Wait for items
		rLockItems()
		if len(clipboardSources)==0{
			continue
		}
		rUnlockItems()

		source, data := dequeueItems()

		if source=="local"{

			conn.Write(protocol.Encode(data))
		}else if source=="remote"{
			clipboard.Set(data)
		}

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

	go readFromRemote()
	go readFromLocal()
	go Process()

	select{} //Sleep forever


}