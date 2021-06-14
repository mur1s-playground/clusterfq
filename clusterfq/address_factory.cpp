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
	char tmp[46];
	memcpy(&tmp, address.c_str(), address.length());
	tmp[address.length()] = '\0';
	address_factory_fix_address_rfc(tmp);
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
	char tmp[46];
	memcpy(&tmp, address.c_str(), address.length());
	tmp[address.length()] = '\0';
	address_factory_fix_address_rfc(tmp);
	string address_tmp(tmp);

	mutex_wait_for(&address_factory_lock);
	map<string, vector<struct address_factory_sender>>::iterator ats = address_to_sender.find(address_tmp);
	for (int i = ats->second.size() - 1; i >= 0; i--) {
		struct address_factory_sender *afs = &ats->second[i];
		if (afs->afst == afst && afs->identity_id == identity_id && afs->sender_id == sender_id) {
			ats->second.erase(ats->second.begin() + i);
			break;
		}
	}
	if (ats->second.size() == 0) {
		address_to_sender.erase(ats);
		server_group_leave(address_tmp);
	}
	mutex_release(&address_factory_lock);
}

vector<struct address_factory_sender> address_factory_sender_get(string address) {
	char tmp[46];
	memcpy(&tmp, address.c_str(), address.length());
	tmp[address.length()] = '\0';
	address_factory_fix_address_rfc(tmp);
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

void address_factory_fix_address_rfc(char* pbar) {
	char part_2[46];
	//memset(&part_2, 'x', 46);
	part_2[45] = '\0';

	int part_2_pos = 44;

	char* end = strchr(pbar, '\0');
	end--;
	int segment_ct = 0;
	int segs = 0;

	bool dbl = false;
	while (end >= pbar) {
		if (*end == ':') {
			if (*(end - 1) == ':') {
				dbl = true;
			} else if (segment_ct > 0) {
				for (int s = segment_ct; s < 4; s++) {
					part_2[part_2_pos] = '0';
					part_2_pos--;
				}
			}
			part_2[part_2_pos] = *end;
			part_2_pos--;
			segment_ct = 0;
			segs++;
		} else {
			part_2[part_2_pos] = *end;
			part_2_pos--;
			segment_ct = (segment_ct + 1) % 4;
		}
		end--;
		if (dbl) break;
	}
	//printf("part_2: %s\n", part_2);

	part_2_pos++;
	if (dbl) {
		char part_1[46];
		//memset(&part_1, 'x', 46);
		int part_1_pos = 44;
		part_1[45] = '\0';
		while (end >= pbar) {
			if (*end == ':') {
				if (segment_ct > 0) {
					for (int s = segment_ct; s < 4; s++) {
						part_1[part_1_pos] = '0';
						part_1_pos--;
					}
				}
				part_1[part_1_pos] = *end;
				part_1_pos--;
				segment_ct = 0;
				segs++;
			} else {
				part_1[part_1_pos] = *end;
				part_1_pos--;
				segment_ct = (segment_ct + 1) % 4;
			}
			end--;
		}
		//printf("part_1: %s\n", part_1);
		part_1_pos++;

		int total_pos = 0;
		memcpy(pbar, &part_1[part_1_pos], 45 - part_1_pos);
		total_pos = 45 - part_1_pos;
		for (int s = segs; s < 8; s++) {
			pbar[total_pos] = '0';
			total_pos++;
			pbar[total_pos] = '0';
			total_pos++;
			pbar[total_pos] = '0';
			total_pos++;
			pbar[total_pos] = '0';
			total_pos++;
			if (s < 7) {
				pbar[total_pos] = ':';
				total_pos++;
			}
		}
		memcpy(&pbar[total_pos], &part_2[part_2_pos], 45 - part_2_pos);
		total_pos += (45 - part_2_pos);
		pbar[total_pos] = '\0';
	} else {
		memcpy(pbar, &part_2[part_2_pos], 45 - part_2_pos);
		pbar[45 - part_2_pos] = '\0';
	}
}