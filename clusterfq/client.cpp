#include "client.h"

#include <vector>
#include <map>
#include <time.h>

#include "mutex.h"

using namespace std;

map<string, struct network_ref> address_to_network = map<string, struct network_ref>();
struct mutex					address_lock;

void client_init() {
	mutex_init(&address_lock);
}

struct network_ref *client_address_add(string address) {
	map<string, struct network_ref>::iterator atn = address_to_network.find(address);
	if (atn == address_to_network.end()) {
		struct network_ref nr;
		network_init(&nr.n);
		network_udp_multicast_socket_client_create(&nr.n, address.c_str(), 1337);
		nr.last_used = time(nullptr);
		address_to_network.insert(pair<string, struct network_ref>(address, nr));
		atn = address_to_network.find(address);
		return &atn->second;
	}
	return &atn->second;
}

void client_address_remove_unused() {
	mutex_wait_for(&address_lock);
	for (map<string, struct network_ref>::const_iterator atn = address_to_network.cbegin(); atn != address_to_network.cend(); ) {
		if (time(nullptr) - atn->second.last_used > 60) {
			network_destroy((struct Network *)&atn->second.n);
			atn = address_to_network.erase(atn);
		} else {
			++atn;
		}
	}
	mutex_release(&address_lock);
}

void client_send_to(string address, unsigned char* packet_buffer, unsigned int packet_length) {
	mutex_wait_for(&address_lock);
	struct network_ref* nr = client_address_add(address);
	nr->last_used = time(nullptr);
	mutex_release(&address_lock);
	nr->n.send(&nr->n, packet_buffer, packet_length);	
}