#include "clusterfq_cl.h"

#include "paramset.h"
#include "socket_interface_cl.h"

namespace ClusterFQ {
	bool ClusterFQ::init(string address, int port) {
		socket_interface_address = address;
		socket_interface_port = port;
		paramset__static_init();
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

	string ClusterFQ::errors() {
		return "";
	}
}