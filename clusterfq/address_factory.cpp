#include "address_factory.h"

#include <sstream>

#include "crypto.h"
#include "server.h"
#include "util.h"

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
	char tmp[46];
	memcpy(&tmp, address.c_str(), address.length());
	tmp[address.length()] = '\0';
	util_ipv6_address_to_normalform(tmp);
	string address_tmp(tmp);

	if (!address_factory_contains(address_tmp, true)) {
		address_factory.push_back(address_tmp);
		vector<struct address_factory_sender> sender = vector<struct address_factory_sender>();
		address_to_sender.insert(pair<string, vector<struct address_factory_sender>>(address_tmp, sender));
		server_group_join(address_tmp);
	}
	map<string, vector<struct address_factory_sender>>::iterator ats = address_to_sender.find(address_tmp);
	struct address_factory_sender afs;
	afs.afst = afst;
	afs.identity_id = identity_id;
	afs.sender_id = sender_id;
	ats->second.push_back(afs);
	mutex_release(&address_factory_lock);
}

void address_factory_remove_address(string address, enum address_factory_sender_type afst, unsigned int identity_id, unsigned int sender_id) {
	if (address.length() > 0) {
		char tmp[46];
		memcpy(&tmp, address.c_str(), address.length());
		tmp[address.length()] = '\0';
		util_ipv6_address_to_normalform(tmp);
		string address_tmp(tmp);

		mutex_wait_for(&address_factory_lock);
		map<string, vector<struct address_factory_sender>>::iterator ats = address_to_sender.find(address_tmp);
		if (ats != address_to_sender.end()) {
			for (int i = ats->second.size() - 1; i >= 0; i--) {
				struct address_factory_sender* afs = &ats->second[i];
				if (afs->afst == afst && afs->identity_id == identity_id && afs->sender_id == sender_id) {
					ats->second.erase(ats->second.begin() + i);
					break;
				}
			}
			if (ats->second.size() == 0) {
				address_to_sender.erase(ats);
				server_group_leave(address_tmp);
			}
		}
		mutex_release(&address_factory_lock);
	}
}

vector<struct address_factory_sender> address_factory_sender_get(string address) {
	char tmp[46];
	memcpy(&tmp, address.c_str(), address.length());
	tmp[address.length()] = '\0';
	util_ipv6_address_to_normalform(tmp);
	string address_tmp(tmp);

	mutex_wait_for(&address_factory_lock);
	map<string, vector<struct address_factory_sender>>::iterator ats = address_to_sender.find(address_tmp);
	if (ats != address_to_sender.end()) {
		vector<struct address_factory_sender> v_afs = ats->second;
		mutex_release(&address_factory_lock);
		return v_afs;
	}
	mutex_release(&address_factory_lock);
	vector<struct address_factory_sender> v_afs = vector<struct address_factory_sender>();
	return v_afs;
}

void address_factory_clear() {
	mutex_wait_for(&address_factory_lock);
	for (int a = 0; a < address_factory.size(); a++) {
		server_group_leave(address_factory[a]);
	}
	mutex_release(&address_factory_lock);
}

bool address_factory_check_multicast_address(string address) {
	if (address.length() < 7) return false;

	const char symbols[17] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f', ':' };

	if (address.at(0) != 'f' || address.at(1) != 'f') return false;
	if (address.at(2) != '0' && address.at(2) != '1') return false;
	if (address.at(3) == 'f') return false;

	if (address.at(address.length() - 1) == ':') return false;

	int nc = 4;
	int segments = 1;
	bool double_found = false;

	for (int i = 4; i < address.length(); i++) {
		bool found = false;
		for (int s = 0; s < 17; s++) {
			if (address.at(i) == symbols[s]) {
				if (address.at(i) == ':') {
					if (nc == 0) return false;
					if (i + 1 < address.length() && address.at(i + 1) == ':') {
						if (double_found) return false;
						double_found == true;
						i++;
					}
					segments++;
					if (segments > 5) return false;
					nc = 0;
				} else {
					nc++;
					if (nc > 4) {
						return false;
					}
				}
				found = true;
				break;
			}
		}
		if (!found) return false;
	}
	if (segments < 2) {
		return false;
	}
	return true;
}
