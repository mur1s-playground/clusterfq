#include "shared_memory_buffer.h"

#include <iostream>

bool shared_memory_buffer_init(struct shared_memory_buffer* smb, string name, int size, int slots, unsigned int drop_skip) {
	smb->name = name;

	smb->size = size;
	smb->slots = slots;
	smb->drop_skip = drop_skip;

	int size_complete = slots * (size + SHAREDMEMORYBUFFER_SLOT_META_SIZE);

#ifdef _WIN32
	smb->h_file = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, size_complete, smb->name.c_str());
	if (smb->h_file == NULL) {
		return false;
	}
	smb->h_buffer = (LPTSTR)MapViewOfFile(smb->h_file, FILE_MAP_ALL_ACCESS, 0, 0, size_complete);
	smb->buffer = (unsigned char*)smb->h_buffer;
#else

#endif
	memset(smb->buffer, 0, size_complete);
	return true;
}

struct shared_memory_buffer* shared_memory_buffer_connect(string name, int size, int slots, unsigned int drop_skip) {
	struct shared_memory_buffer* smb = new shared_memory_buffer();
	smb->name = name;
	smb->size = size;
	smb->slots = slots;
	smb->drop_skip = drop_skip;

	int size_complete = slots * (size + SHAREDMEMORYBUFFER_SLOT_META_SIZE);

#ifdef _WIN32
	smb->h_file = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, smb->name.c_str());
	if (smb->h_file == NULL) {
		delete smb;
		return nullptr;
	}
	smb->h_buffer = (LPTSTR)MapViewOfFile(smb->h_file, FILE_MAP_ALL_ACCESS, 0, 0, size_complete);
	smb->buffer = (unsigned char*)smb->h_buffer;
#else

#endif
	return smb;
}

void shared_memory_buffer_set_new(struct shared_memory_buffer* smb, int slot, bool new_) {
	smb->buffer[slot * (smb->size + SHAREDMEMORYBUFFER_SLOT_META_SIZE) + SHAREDMEMORYBUFFER_SLOT_META_NEW] = new_;
}

bool shared_memory_buffer_is_new(struct shared_memory_buffer* smb, int slot) {
	return smb->buffer[slot * (smb->size + SHAREDMEMORYBUFFER_SLOT_META_SIZE) + SHAREDMEMORYBUFFER_SLOT_META_NEW] == 1;
}

int shared_memory_buffer_write_slot(struct shared_memory_buffer* smb, int slot, unsigned char* data, unsigned int data_len) {
	int slot_c = slot;
	if (shared_memory_buffer_is_new(smb, slot)) {
		if (smb->drop_skip > 0) {
			slot_c = (slot + smb->drop_skip) % smb->slots;
		} else {
			return -1;
		}
	}
	memcpy(&smb->buffer[slot_c * (smb->size + SHAREDMEMORYBUFFER_SLOT_META_SIZE) + SHAREDMEMORYBUFFER_SLOT_DATA], data, data_len);
	shared_memory_buffer_set_new(smb, slot_c, true);
	return slot_c;
}

bool shared_memory_buffer_read_slot(struct shared_memory_buffer* smb, int slot, unsigned char* data) {
	if (shared_memory_buffer_is_new(smb, slot)) {
		memcpy(data, &smb->buffer[slot * (smb->size + SHAREDMEMORYBUFFER_SLOT_META_SIZE) + SHAREDMEMORYBUFFER_SLOT_DATA], smb->size);
		shared_memory_buffer_set_new(smb, slot, false);
		return true;
	} else {
		return false;
	}
}