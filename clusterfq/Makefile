all: clusterfq.o address_factory.o client.o contact.o contact_stats.o crypto.o ecdh.o group.o identity.o message.o mutex.o network.o packetset.o server.o shell.o socket_interface.o thread.o util.o
	g++ -std=c++11 clusterfq.o address_factory.o client.o contact.o contact_stats.o crypto.o ecdh.o group.o identity.o message.o mutex.o network.o packetset.o server.o shell.o socket_interface.o thread.o util.o -lssl -lcrypto -lpthread -o clusterfq

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
	
shell.o:
	g++ -std=c++11 -c shell.cpp

socket_interface.o:
	g++ -std=c++11 -c socket_interface.cpp
	
thread.o:
	g++ -std=c++11 -c thread.cpp
	
util.o:
	g++ -std=c++11 -c util.cpp

clean:
	rm *.o;
	rm clusterfq
