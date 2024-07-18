#include <queue>
#include <tuple>
#include <string>
#include <map>
#include <unordered_map>
#include <condition_variable>
#include <asio_c.h>
#include <shared_mutex>
#include <atomic>
#include <set>
#include <csignal>
#include <thread>
#include "clip/clip.hpp"
#include "protocol/protocol.hpp"

enum Source {
	LOCAL,
	REMOTE
};

typedef std::pair<Source, std::string> clipboardItem;
std::queue<clipboardItem> clipboardItems;
std::mutex clipboardItemsMutex;
std::condition_variable clipboardItemsCV;

std::unordered_map<std::string, int> recievedItems;

protocol::Selection currentSelection;

typedef struct {
	AsioConn* conn = NULL;
	std::mutex mu;
	std::atomic_flag stop = ATOMIC_FLAG_INIT;
} Conn;

std::map<int,Conn> clients;
std::shared_mutex clientsMutex;
std::set<int> clientIds = {0,1,2,3,4,5,6,7,8,9,10}; //Good enough for now

#ifdef CLIENT
	Conn clientConn;
#endif

void queueItem(Source source, std::string& data){
	std::unique_lock lk(clipboardItemsMutex);
	clipboardItems.push({source, data});
	clipboardItemsCV.notify_one();
}

clipboardItem dequeueItem(){
	std::unique_lock lk(clipboardItemsMutex);
	clipboardItemsCV.wait(lk, [&] { return !clipboardItems.empty();});
	auto item = clipboardItems.front();
	clipboardItems.pop();

	return item;

}

void reconnectToServer(){
	#ifdef CLIENT
		asio_close(clientConn.conn);
		clientConn.conn=asio_connect(1);
	#endif
}

void readFromLocal(){
	for(;;){
		#ifndef __APPLE__
			clip::Changed();
		#endif
		
		auto selection=clip::Get();
		
		//Don't want to send the same thing twice --- may not be needed with clipboardHasChanged (can't usec on MacOS though)
		if(selection==currentSelection){
			continue;
		}
		
		printf("Get change!\n");
		currentSelection=selection;
		
		auto str=protocol::Encode(selection);
		queueItem(LOCAL, str);	
		
	}
}
//Server --- each connection gets it's own readFromRemote. Process sends each string to all connected clients.
//Client --- the one connection get it's own readFromRemote. Process sends it to it's own connection

void readFromRemote(Conn& conn){
	
	bool err;
	char* buf;
	int size;
	while(true){
		std::unique_lock lk(conn.mu);
		if (conn.stop.test()){
			break;
		}

		asio_read(conn.conn,&buf, &size, &err);
		if (err){
			#ifdef CLIENT
				reconnectToServer();
				continue;
			#else
				break;
			#endif
		}

		std::string str(buf, size);
		queueItem(REMOTE, str);
	}
}

void readFromRemote(int id){
	clientsMutex.lock_shared();
	auto& conn=clients.at(id);
	clientsMutex.unlock_shared();

	readFromRemote(conn);

	conn.mu.lock();
	asio_close(conn.conn);
	conn.conn=NULL;
	conn.mu.unlock();

	clientsMutex.lock();
	clients.erase(id);
	clientIds.insert(id);
	clientsMutex.unlock();
}

void Process(){
	for(;;){
		auto [source, data] = dequeueItem();
		auto selection = protocol::Decode(data);	
	
		//Proto-ACK functionality. If you recieve selection, don't send the same selection (will just lead to a loop). Acknowledge you recieved it by ignoring it in the clipboard.
		if (source==LOCAL){
			auto& count=recievedItems.try_emplace(data, 0).first->second;
			if (count>0){
				count--;
				continue;
			}

			printf("Sending...\n");
			
			#ifdef CLIENT
				bool err=true;

				while(err){
					clientConn.mu.lock();
					asio_write(clientConn.conn, data.data(), data.size(), &err);	
					if (err){
						reconnectToServer();
					}
					clientConn.mu.unlock();
				}
			#else
				clientsMutex.lock_shared();
				for (auto& [key, val]: clients){
					bool err;

					val.mu.lock();
					asio_write(val.conn, data.data(), data.size(), &err);
					if (err){
						val.stop.test_and_set();
					}
					val.mu.unlock();
				}
				clientsMutex.unlock_shared();
			#endif

			printf("Sent!\n");


		}else if (source==REMOTE){
			printf("Recieved!\n");
			recievedItems[data]++;

			printf("Setting!\n");
			clip::Set(selection);
			printf("Set!\n");

		}
	}
}

int main(){
	signal(SIGTERM, [](int) { raise(SIGINT); });

	clip::Init();
	std::thread t1(readFromLocal);
	std::thread t2(Process);

	#ifdef CLIENT
		clientConn.mu.lock();
		reconnectToServer();
		clientConn.mu.unlock();

		std::thread t3((void(*)(Conn&))readFromRemote, std::ref(clientConn));
		t3.join();
	#else
		auto server=asio_server_init(1);
		for(;;){
			clientsMutex.lock();
			auto it=clientIds.begin();
			auto id=*it;
			auto& client = clients[id];
			clientIds.erase(it);
			clientsMutex.unlock();

			client.mu.lock();
			client.conn=asio_server_accept(server);
			client.mu.unlock();

			std::thread((void(*)(int))readFromRemote,id).detach();
		}
			
	#endif

	clip::Run();
	t1.join();
	t2.join();
}
