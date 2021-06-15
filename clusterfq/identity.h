#pragma once

#include <string>
#include <vector>

#include <time.h>

#include "crypto.h"

#include "contact.h"
#include "group.h"


using namespace std;

struct identity {
	unsigned int			id;

	string					name;

	vector<struct Key>		keys;
	time_t					key_latest_creation;

	vector<struct contact>	contacts;
	vector<struct group>	groups;
};

extern vector<struct identity>		identities;

void identity_create(struct identity *i, string name);
void identity_create_fs(struct identity* i);

void identity_load(unsigned int id);
void identity_load(struct identity* i, unsigned int id);

void identity_migrate_key(struct identity* i, unsigned int bits);
void identity_remove_obsolete_keys(struct identity* i);

void identity_save_keys(struct identity* i);
void identity_save_keys(struct identity* i, string base_dir);
void identity_load_keys(struct identity* i, string base_dir);

void identities_load();
void identities_list();

struct identity* identity_get(unsigned int id);

void identity_contact_add(unsigned int id, struct contact *c);
void identity_share(unsigned int id, string name_to);
void identity_contact_list(unsigned int id);