#include <exception>
#include <queue>
#include <tuple>
#include <string>
#include <map>
#include <condition_variable>
#include <asio_c.h>
#include <shared_mutex>
#include <atomic>
#include <set>
#include <csignal>
#include <thread>

enum Source {
	LOCAL,
	REMOTE
};

typedef std::pair<Source, std::string> clipboardItem;
std::queue<clipboardItem> clipboardItems;
std::mutex clipboardItemsMutex;
std::condition_variable clipboardItemsCV;

std::map<Selection, int> recievedItems;

Selection currentSelection;

typedef struct {
	AsioConn* conn = NULL;
	std::mutex mu;
	std::atomic_flag stop = ATOMIC_FLAG_INIT;
} Conn;

std::map<int,Conn> clients;
std::shared_mutex clientsMutex;
std::set<int> clientIds = {0,1,2,3,4,5,6,7,8,9,10}; //Good enough for now

#ifdef CLIENT
	Conn* clientConn;
#endif

void queueItem(Source source, std::string& data){
	std::unique_lock lk(clipboardItemsMutex);
	clipboardItems.push({source, data});
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
		globalConn.conn=asio_connect(1);
	#endif
}

void readFromLocal(){
	for(;;){
		#ifndef __APPLE__
			clipboard::HasChanged();
		#endif

		auto selection=clipboard::Get();
		
		//Don't want to send the same thing twice --- may not be needed with clipboardHasChanged (can't usec on MacOS though)
		if(selection==currentSelection){
			continue;
		}
		
		currentSelection=selection;

		queueItem(LOCAL, protocol::Encode(selection));	
		
	}
}
//Server --- each connection gets it's own readFromRemote. Process sends each string to all connected clients.
//Client --- the one connection get it's own readFromRemote. Process sends it to it's own connection

void readFromRemote(Conn& conn){
	
	bool err;
	char* buf;
	int size;
	while(true){
		if (conn.stop.test()){
			break;
		}
		{
			std::unique_lock lk(conn.mu);
			asio_read(conn.conn,&buf, &size, &err);
			if (err){
				#ifdef CLIENT
					reconnectToServer();
					continue;
				#else
					break;
				#endif
			}

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

	asio_close(conn.conn);
	clientsMutex.lock();
	clients.erase(id);
	clientIds.insert(id);
	clientsMutex.unlock();
}

void Process(){
	for(;;){
		auto [source, data] = dequeueItem();
		auto selection, err = protocol::Decode(data);	
	
		//Proto-ACK functionality. If you recieve selection, don't send the same selection (will just lead to a loop). Acknowledge you recieved it by ignoring it in the clipboard.
		if (source==LOCAL){
			auto& count=recievedItems.try_or_emplace(selection, 0).first.second;
			if (count>0){
				count--;
				continue;
			}

			printf("Sending...\n");
			
			#ifdef CLIENT
				bool err=true;

				while(err){
					std::unique_lock lk(mu);
					asio_write(globalConn.conn, data.data(), data.size(), &err);	
					if (err){
						reconnectToServer();
					}
				}
			#else
				clientsMutex.lock_shared();
				for (auto& [key, val]: clients){
					bool err;
					asio_write(val.conn, data.data(), data.size(), &err);
					if (err){
						val.stop.test_and_set();
					}
				}
				clientsMutex.unlock_shared();
			#endif

			printf("Sent!\n");


		}else if (source==REMOTE){
			printf("Recieved!\n");
			recievedItems[selection]++;

			printf("Setting!\n");
			clipboard::Set(selection);
			printf("Set!\n");

		}
	}
}

int main(){
	signal(SIGTERM, [](int) { raise(SIGINT); });

	std::thread t1(readFromLocal);
	std::thread t2(Process);

	#ifdef CLIENT
		reconnectToServer();
		std::thread t3(readFromRemote, clientConn);
		t3.join();
	#else
		auto server=asio_server_init(1);
		for(;;){
			clientsMutex.lock();
			auto& id=clientIds.begin();
			auto& client = clients[*id];
			clientIds.erase(id);
			client.conn=asio_server_accept(server);
			std::thread(readFromRemote,id);
		}
			
	#endif

	clipboard::Wait();
	t1.join();
	t2.join();
}
