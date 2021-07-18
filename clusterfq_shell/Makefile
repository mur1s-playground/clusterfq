all: audio_device.o audio_dst.o audio_source.o clusterfq_shell.o const_defs.o mutex.o shell.o smb_interface.o thread.o util.o
	g++ -std=c++11 audio_device.o audio_dst.o audio_source.o clusterfq_shell.o const_defs.o mutex.o shell.o smb_interface.o thread.o util.o -lpthread -lasound -L./ -lclusterfq_cl -o clusterfq_shell

audio_device.o:
	g++ -std=c++11 -c audio_device.cpp

audio_dst.o:
	g++ -std=c++11 -c audio_dst.cpp

audio_source.o:
	g++ -std=c++11 -c audio_source.cpp

clusterfq_shell.o:
	g++ -std=c++11 -c clusterfq_shell.cpp
	
const_defs.o:
	g++ -std=c++11 -c ../clusterfq_cl/const_defs.cpp
	
mutex.o:
	g++ -std=c++11 -c ../clusterfq/mutex.cpp
	
shell.o:
	g++ -std=c++11 -c shell.cpp
	
smb_interface.o:
	g++ -std=c++11 -c smb_interface.cpp
	
thread.o:
	g++ -std=c++11 -c ../clusterfq/thread.cpp
	
util.o:
	g++ -std=c++11 -c ../clusterfq/util.cpp

clean:
	rm *.o;
	rm clusterfq_shell
