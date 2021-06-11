#pragma once

#include <string>
#include <vector>

#include "crypto.h"

#include "contact.h"
#include "group.h"


using namespace std;

struct identity {
	unsigned int			id;

	string					name;

	vector<struct Key>		keys;

	vector<struct contact>	contacts;
	vector<struct group>	groups;
};

extern vector<struct identity>		identities;

void identity_create(struct identity *i, string name);
void identity_create_fs(struct identity* i);

void identity_load(unsigned int id);
void identity_load(struct identity* i, unsigned int id);

void identities_load();
void identities_list();

struct identity* identity_get(unsigned int id);

void identity_contact_add(unsigned int id, struct contact *c);
void identity_share(unsigned int id, string name_to);
void identity_contact_list(unsigned int id);