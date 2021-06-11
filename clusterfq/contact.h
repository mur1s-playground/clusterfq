#pragma once

#include <string>
#include <vector>
#include <map>

#include "crypto.h"
#include "packetset.h"
#include "message.h"
#include "mutex.h"
#include "identity.h"

using namespace std;

struct contact {
	unsigned int					id;

	string							name;

	struct Key						pub_key;
	string							address;

	string							address_rev;

	struct Key						session_key;

	vector<struct message_meta *>	outgoing_messages;
	struct mutex					outgoing_messages_lock;

	map<string, struct packetset>	incoming_packetsets;
};

void			contact_static_init();
struct contact* contact_static_get_pending();

void contact_create_open(struct contact* c, string name_placeholder, string address_rev);
void contact_create(struct contact* c, string name, string pubkey, string address);

struct contact* contact_get(vector<struct contact> *contacts, unsigned int id);

void contact_save(struct contact* c, string path);
void contact_load(struct contact* c, unsigned int identity_id, unsigned int id, string path);
void contact_dump(struct contact* c);

void contact_add_message(struct contact* c, struct message_meta *mm);
bool contact_process_message(struct identity *i, struct contact *c, unsigned char *packet_buffer_packet, unsigned int packet_len);