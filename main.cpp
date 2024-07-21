#include <mutex>
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

//Concurrency guarentee --- When a thread is finished reading/modifying a Conn.conn, it must either be valid or NULL.
typedef struct {
	AsioConn* conn = NULL;
	std::shared_mutex mu;
	std::atomic_flag stop = ATOMIC_FLAG_INIT;
} Conn;

std::map<int,Conn> clients;
std::shared_mutex clientsMutex;
std::set<int> clientIds = {0,1,2,3,4,5,6,7,8,9,10}; //Good enough for now

#ifdef CLIENT
	Conn clientConn;
#endif

void queueItem(Source source, std::string& data){
	clipboardItemsMutex.lock();
	clipboardItems.push({source, data});
	clipboardItemsMutex.unlock();
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
		std::unique_lock lk(clientConn.mu);
		printf("Reconnecting!\n");
		asio_close(clientConn.conn);
		clientConn.conn=asio_connect(1);
		printf("Done reconnecting!\n");
	#endif
}

void readFromLocal(){
	for(;;){
		clip::Changed();

		
		auto selection=clip::Get();
		
		//Don't want to send the same thing twice --- may not be needed with clipboardHasChanged (can't usec on MacOS though)
		if(selection==currentSelection){
			continue;
		}
		

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

		if (conn.stop.test()){
			break;
		}

		printf("Reading!\n");
		conn.mu.lock_shared();
		asio_read(conn.conn,&buf, &size, &err);
		conn.mu.unlock_shared();
		printf("Finished reading!\n");
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
			if (recievedItems.contains(data)){ //data will only be in recievedItems if count>0; ie, there are clipboard items that haven't been ACKed yet
				auto& count=recievedItems[data];
				count--;
				if (count<=0){ //Should be removed now
					recievedItems.erase(data);
				}

				continue;
			}

			printf("Sending...\n");
			
			#ifdef CLIENT
				bool err=true;

				while(err){
					{
					std::shared_lock lk(clientConn.mu);
					asio_write(clientConn.conn, data.data(), data.size(), &err);					
					}

					if (err){
						reconnectToServer();

					}
				}
			#else
				clientsMutex.lock_shared();
				for (auto& [key, val]: clients){
					bool err;
					
					{
					std::shared_lock lk(val.mu);
					asio_write(val.conn, data.data(), data.size(), &err);
					if (err){
						val.stop.test_and_set();
					}
					}
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
		reconnectToServer();


		std::thread t3((void(*)(Conn&))readFromRemote, std::ref(clientConn));
	#else
		std::thread t3([]{}); //Just a dummy one to match the client
		std::thread([]{
			auto server=asio_server_init(1);
			for(;;){
				auto conn=asio_server_accept(server);
				//You must wait for conn to be valid before assigning it to a client --- we implcitly assume that any client in the clients map must be valid (ie, client.conn is valid)
				clientsMutex.lock();
				auto it=clientIds.begin();
				auto id=*it;
				auto& client = clients[id];
				clientIds.erase(it);
				clientsMutex.unlock();

				{
				std::unique_lock lk(client.mu);
				client.conn=conn;
				}

				std::thread((void(*)(int))readFromRemote,id).detach();
			}
		}).detach();
			
	#endif

	clip::Run();

	t3.join();
	t1.join();
	t2.join();
}
