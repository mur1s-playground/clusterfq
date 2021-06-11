#pragma once

#include <string>
#include <vector>
#include <map>
#include <time.h>

#include "crypto.h"
#include "packetset.h"
#include "message.h"
#include "mutex.h"
#include "identity.h"

#include "contact_stats.h"

using namespace std;

struct contact {
	unsigned int					id;

	string							name;

	struct Key						pub_key;
	string							address;

	string							address_rev;

	struct Key						session_key;
	time_t							session_established;

	vector<struct message_meta *>	outgoing_messages;
	struct mutex					outgoing_messages_lock;

	map<string, struct packetset>	incoming_packetsets;

	struct contact_stats*			cs;
};

void			contact_static_init();
struct contact* contact_static_get_pending();

void contact_create_open(struct contact* c, string name_placeholder, string address_rev);
void contact_create(struct contact* c, string name, string pubkey, string address);

struct contact* contact_get(vector<struct contact> *contacts, unsigned int id);

void contact_save(struct contact* c, string path);
void contact_load(struct contact* c, unsigned int identity_id, unsigned int id, string path);

/* LOAD/SAVE SUB */
void contact_pubkey_save(struct contact* c, unsigned int identity_id);
void contact_pubkey_save(struct contact* c, string path);
void contact_pubkey_load(struct contact* c, string path);

void contact_sessionkey_save(struct contact* c, unsigned int identity_id);
void contact_sessionkey_save(struct contact* c, string path);
void contact_sessionkey_load(struct contact* c, string path);

void contact_stats_save(struct contact* c, unsigned int identity_id);
void contact_stats_save(struct contact* c, string path);
void contact_stats_load(struct contact* c, string path);
/* ------------- */


void contact_dump(struct contact* c);

void contact_add_message(struct contact* c, struct message_meta *mm, bool prepend = false);
bool contact_process_message(struct identity *i, struct contact *c, unsigned char *packet_buffer_packet, unsigned int packet_len);