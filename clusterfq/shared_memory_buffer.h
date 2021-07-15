#pragma once

#include <string>

#ifdef _WIN32
#include <windows.h>
#else

#endif

using namespace std;

#define SHAREDMEMORYBUFFER_SLOT_META_SIZE 1
#define SHAREDMEMORYBUFFER_SLOT_META_NEW 0
#define SHAREDMEMORYBUFFER_SLOT_DATA 1

struct shared_memory_buffer {
	string name;

#ifdef _WIN32
	HANDLE h_file;
	LPCTSTR h_buffer;
#else
	int shm_id;
	void* h_buffer;
#endif

	unsigned char* buffer;

	int size;
	int slots;

	int drop_skip;
};

bool shared_memory_buffer_init(struct shared_memory_buffer *smb, string name, int size, int slots, unsigned int drop_skip);
struct shared_memory_buffer* shared_memory_buffer_connect(string name, int size, int slots, unsigned int drop_skip);

int shared_memory_buffer_write_slot(struct shared_memory_buffer* smb, int slot, unsigned char* data, unsigned int data_len);
bool shared_memory_buffer_read_slot(struct shared_memory_buffer* smb, int slot, unsigned char* data);