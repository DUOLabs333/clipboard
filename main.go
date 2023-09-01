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
	"reflect"
	clipboard "clipboard/clip"
	"av-forward/server"
	"clipboard/protocol"

)


var clipboardItems=make(chan [2][]byte)

var recievedItems map[string]int = make(map[string]int)
var recievedItemsLock sync.RWMutex

var currentSelection protocol.Selection
var currentSelectionLock sync.RWMutex

var conn io.ReadWriteCloser
var connLock sync.RWMutex

var isServer bool
var hostAddr string

func queueItem(src string,data []byte){
	clipboardItems <- [2][]byte{[]byte(src),data};
}

func dequeueItem() (src string, data []byte){
	item:=<-clipboardItems
	src_byte, data:= item[0], item[1]
	
	src=string(src_byte)
	
	return
}

func reconnectToServer(){
	connLock.Lock()
	defer connLock.Unlock()
	
	if !isServer{ //Only clients need to reconnect to a server --- a server can't reconnect to itself
		for(true){
			if conn!=nil{ //If nil, you definitely need to (re)connect
				_,err:=conn.Write([]byte("a\n")) //We can write this because the server will just ignore it with a decode error
				if err==nil{
					break
				}
				conn.Close()
			}
		
			for(true){
				var err error
				conn,err=net.Dial("tcp",fmt.Sprintf("%s:%d",hostAddr,8001))
				if err==nil{
					break
				}
			}
			
		}
		fmt.Println("Reconnected!")
	}
}

func readFromLocal(){
	for {
		if runtime.GOOS!="darwin"{
			clipboard.ClipboardHasChanged()
		}
		
		selection:=clipboard.Get();

		//Don't want to send the same thing twice --- may not be needed with clipboardHasChanged (can't use on MacOS though)
		currentSelectionLock.RLock()
		if reflect.DeepEqual(selection,currentSelection){
			currentSelectionLock.RUnlock()
			continue
		}
		currentSelectionLock.RUnlock()

		currentSelectionLock.Lock()
		currentSelection=selection
		currentSelectionLock.Unlock()

		queueItem("local",protocol.Encode(selection));

	}

}


func readFromRemote(){
	scanner:=bufio.NewReader(conn)

	fmt.Println("Hello!")
	for {
		line:=make([]byte,0)
		
		frag:=make([]byte,0)
		incomplete:=true
		var err error
		for (incomplete){ //Wait for whole line
		
			connLock.RLock()
			frag, incomplete, err =scanner.ReadLine()
			connLock.RUnlock()
			
			if err!=nil{
				reconnectToServer()
				connLock.RLock()
				scanner.Reset(conn) //Set scanner to new non-closed connection
				connLock.RUnlock()
			}
			line=append(line,frag...) //Reading is done
		}	
		if len(line)==0{
			continue
		}
		
		queueItem("remote",line)
	}
}

func Process(){
	for{
		//Wait for items
		source, data := dequeueItem()
		
		selection, err := protocol.Decode(data)
		if (err!=nil){
			fmt.Println("Decode Error:",err)
		}
		
		hash := protocol.Hash(selection)
		
		if source=="local"{
		
			//Proto-ACK functionality. If you recieve selection, don't send the same selection (will just lead to a loop). Acknowledge you recieved it by ignoring it in the clipboard.
			recievedItemsLock.Lock()
			if recievedItems[hash]>0{
				recievedItems[hash]-=1
				recievedItemsLock.Unlock()
				continue
			}
			recievedItemsLock.Unlock()
			
			fmt.Println("Sending...")
			
			for (true){  
				connLock.RLock()
				_,err:=conn.Write(data)
				connLock.RUnlock()
				if (err!=nil){
					reconnectToServer()
				}else{
					break //Writing is done, exit
				}
			}
			fmt.Println("Sent!")
			
			
		}else if source=="remote"{
		
			fmt.Println("Recieved!")
	
			recievedItemsLock.Lock()
			recievedItems[hash]+=1
			recievedItemsLock.Unlock()
			
			fmt.Println("Setting...")
			clipboard.Set(selection)
			fmt.Println("Set!")
			
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

	
	if runtime.GOOS=="darwin"{
		isServer=true
	}else if runtime.GOOS=="linux"{
		isServer=false
	}
	
	
	var host_help_text string
	
	if isServer{
		host_help_text="server will be running on"
	}else{
		host_help_text="client will be connecting to"
	}

	flag.StringVar(&hostAddr,"host","127.0.0.1",fmt.Sprintf("IP where the %s",host_help_text))
	flag.Parse()

	if isServer{
		server:=new(server.Server)
		server.Start(hostAddr,8001)
		conn=server
	}
	
	reconnectToServer()

	//Go from busy loop to channels

	runtime.LockOSThread() //So Init and Wait are in the same thread
	clipboard.Init()
	
	go readFromRemote()
	go Process()
	go readFromLocal()

	clipboard.Wait()


}
