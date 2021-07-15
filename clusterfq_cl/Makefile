all: clusterfq_cl.o const_defs.o mutex.o network.o paramset.o shared_memory_buffer.o socket_interface_cl.o thread.o util.o
	g++ -std=c++11 clusterfq_cl.o const_defs.o mutex.o network.o paramset.o shared_memory_buffer.o socket_interface_cl.o thread.o util.o -lpthread -shared -o libclusterfq_cl.so

clusterfq_cl.o:
	g++ -fPIC -std=c++11 -c clusterfq_cl.cpp
	
const_defs.o:
	g++ -fPIC -std=c++11 -c const_defs.cpp
	
mutex.o:
	g++ -fPIC -std=c++11 -c ../clusterfq/mutex.cpp
	
network.o:
	g++ -fPIC -std=c++11 -c ../clusterfq/network.cpp
	
paramset.o:
	g++ -fPIC -std=c++11 -c paramset.cpp	
	
shared_memory_buffer.o:
	g++ -fPIC -std=c++11 -c ../clusterfq/shared_memory_buffer.cpp
	
socket_interface_cl.o:
	g++ -fPIC -std=c++11 -c socket_interface_cl.cpp
	
thread.o:
	g++ -fPIC -std=c++11 -c ../clusterfq/thread.cpp
	
util.o:
	g++ -fPIC -std=c++11 -c ../clusterfq/util.cpp

clean:
	rm *.o;
	rm libclusterfq_cl.so
