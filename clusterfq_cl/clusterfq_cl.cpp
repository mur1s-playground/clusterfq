#include "clusterfq_cl.h"

#include "paramset.h"
#include "socket_interface_cl.h"
#include "../clusterfq/shared_memory_buffer.h"

namespace ClusterFQ {
	bool ClusterFQ::init(string address, int port) {
		socket_interface_address = address;
		socket_interface_port = port;
		paramset__static_init();
		return true;
	}

	string ClusterFQ::query(const char* module, const char* module_action, int paramset_id) {
		return socket_interface_query(module, module_action, paramset_id);
	}

	int ClusterFQ::paramset_create() {
		return paramset__create();
	}

	bool ClusterFQ::paramset_param_add(int id, const char* param, int value) {
		return paramset__param_add(id, param, value);
	}

	bool ClusterFQ::paramset_param_add(int id, const char* param, long long value) {
		return paramset__param_add(id, param, value);
	}

	bool ClusterFQ::paramset_param_add(int id, const char* param, string value) {
		return paramset__param_add(id, param, value);
	}


	void *ClusterFQ::shared_memory_buffer_connect_i(string name, int size, int slots, unsigned int drop_skip) {
		return (void *)shared_memory_buffer_connect(name, size, slots, drop_skip);
	}

	int ClusterFQ::shared_memory_buffer_write_slot_i(void *smb, int slot, unsigned char* data, unsigned int data_len) {
		return shared_memory_buffer_write_slot((struct shared_memory_buffer*)smb, slot, data, data_len);
	}

	bool ClusterFQ::shared_memory_buffer_read_slot_i(void *smb, int slot, unsigned char* data) {
		return shared_memory_buffer_read_slot((struct shared_memory_buffer *)smb, slot, data);
	}

	string ClusterFQ::errors() {
		return "";
	}
}