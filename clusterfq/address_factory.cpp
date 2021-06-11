#include "address_factory.h"

#include <sstream>

#include "crypto.h"
#include "server.h"

#ifdef _WIN32
#else
#include <cstring>
#endif

map<string, vector<struct address_factory_sender>>	address_to_sender = map<string, vector<struct address_factory_sender>>();
vector<string>										address_factory = vector<string>();

struct mutex										address_factory_lock;

void address_factory_init() {
	mutex_init(&address_factory_lock);
}

bool address_factory_contains(string address, bool keep_lock) {
	mutex_wait_for(&address_factory_lock);
	bool found = false;
	for (int a = 0; a < address_factory.size(); a++) {
		if (strstr(address_factory[a].c_str(), address.c_str()) != nullptr) {
			found = true;
			break;
		}
	}
	if (!keep_lock) mutex_release(&address_factory_lock);
	return found;
}

string address_factory_get_unique() {
	string address_base = "ff12::1337:";

	const char symbols[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
	while (true) {
		stringstream address;
		address << address_base;

		//TODO: improve
		unsigned char* addr_rand = (unsigned char *)crypto_random(10);
		for (int i = 0; i < 3; i++) {
			for (int j = 0; j < 4; j++) {
				address << symbols[addr_rand[i * 4 + j] % 16];
			}
			if (i < 2) address << ":";
		}
		free(addr_rand);

		if (!address_factory_contains(address.str())) {
			return address.str();
		}
	}
}

void address_factory_add_address(string address, enum address_factory_sender_type afst, unsigned int identity_id, unsigned int sender_id) {
	if (!address_factory_contains(address, true)) {
		address_factory.push_back(address);
		vector<struct address_factory_sender> sender = vector<struct address_factory_sender>();
		address_to_sender.insert(pair<string, vector<struct address_factory_sender>>(address, sender));
		server_group_join(address);
	}
	map<string, vector<struct address_factory_sender>>::iterator ats = address_to_sender.find(address);
	struct address_factory_sender afs;
	afs.afst = afst;
	afs.identity_id = identity_id;
	afs.sender_id = sender_id;
	ats->second.push_back(afs);
	mutex_release(&address_factory_lock);
}

vector<struct address_factory_sender> *address_factory_sender_get(string address) {
	map<string, vector<struct address_factory_sender>>::iterator ats = address_to_sender.find(address);
	if (ats != address_to_sender.end()) {
		return &ats->second;
	}
	return nullptr;
}

void address_factory_clear() {
	mutex_wait_for(&address_factory_lock);
	for (int a = 0; a < address_factory.size(); a++) {
		server_group_leave(address_factory[a]);
	}
	mutex_release(&address_factory_lock);
}