#pragma once

#include <string>
#include <vector>
#include <map>

#include "mutex.h"

using namespace std;

enum address_factory_sender_type {
	AFST_CONTACT,
	AFST_GROUP
};

struct address_factory_sender {
	enum address_factory_sender_type	afst;
	unsigned int						identity_id;
	unsigned int						sender_id;
};

extern map<string, vector<struct address_factory_sender>>		address_to_sender;
extern vector<string>											address_factory;
extern struct mutex												address_factory_lock;

bool address_factory_contains(string address, bool keep_lock = false);

void address_factory_init();
string address_factory_get_unique();

void address_factory_add_address(string address, enum address_factory_sender_type afst, unsigned int identity_id, unsigned int sender_id);
void address_factory_remove_address(string address, enum address_factory_sender_type afst, unsigned int identity_id, unsigned int sender_id);

vector<struct address_factory_sender> address_factory_sender_get(string address);
void address_factory_clear();

void address_factory_fix_address_rfc(char *potentially_broken_address_representation);
bool address_factory_check_multicast_address(string address);