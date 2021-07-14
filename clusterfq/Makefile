all: clusterfq.o address_factory.o client.o contact.o contact_stats.o crypto.o ecdh.o group.o identity.o lossy_pipe.o message.o mutex.o network.o packetset.o server.o shared_memory_buffer.o socket_interface.o thread.o util.o
	g++ -std=c++11 clusterfq.o address_factory.o client.o contact.o contact_stats.o crypto.o ecdh.o group.o identity.o lossy_pipe.o message.o mutex.o network.o packetset.o server.o shared_memory_buffer.o socket_interface.o thread.o util.o -lssl -lcrypto -lpthread -o clusterfq

clusterfq.o:
	g++ -std=c++11 -c clusterfq.cpp
	
address_factory.o:
	g++ -std=c++11 -c address_factory.cpp
	
client.o:
	g++ -std=c++11 -c client.cpp	
	
contact.o:
	g++ -std=c++11 -c contact.cpp
	
contact_stats.o:
	g++ -std=c++11 -c contact_stats.cpp
	
crypto.o: 
	g++ -std=c++11 -c crypto.cpp

ecdh.o:
	g++ -std=c++11 -c ecdh.cpp

group.o:
	g++ -std=c++11 -c group.cpp

identity.o:
	g++ -std=c++11 -c identity.cpp
		
lossy_pipe.o:
	g++ -std=c++11 -c lossy_pipe.cpp
		
message.o:
	g++ -std=c++11 -c message.cpp
	
mutex.o:
	g++ -std=c++11 -c mutex.cpp
	
network.o:
	g++ -std=c++11 -c network.cpp

packetset.o:
	g++ -std=c++11 -c packetset.cpp

server.o:
	g++ -std=c++11 -c server.cpp
	
shared_memory_buffer.o:
	g++ -std=c++11 -c shared_memory_buffer.cpp
	
socket_interface.o:
	g++ -std=c++11 -c socket_interface.cpp
	
thread.o:
	g++ -std=c++11 -c thread.cpp
	
util.o:
	g++ -std=c++11 -c util.cpp

clean:
	rm *.o;
	rm clusterfq
