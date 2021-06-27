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
#include "socket_interface.h"

#include "contact_stats.h"

using namespace std;

struct contact {
	unsigned int					id;

	string							name;

	unsigned int					identity_key_id;

	struct Key						pub_key;
	string							address;

	string							address_rev;
	string							address_rev_migration;
	time_t							address_rev_established;

	struct Key						session_key;
	time_t							session_established;

	struct Key						session_key_inc;
	struct mutex					session_key_inc_lock;

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

void contact_delete(struct contact* c, unsigned int identity_id);

/* LOAD/SAVE SUB */
void contact_identity_key_id_save(struct contact* c, unsigned int identity_id);
void contact_identity_key_id_save(struct contact* c, string path);
void contact_identity_key_id_load(struct contact* c, string path);

void contact_address_save(struct contact* c, unsigned int identity_id);
void contact_address_save(struct contact* c, string path);
void contact_address_load(struct contact* c, string path);

void contact_address_rev_save(struct contact* c, unsigned int identity_id);
void contact_address_rev_save(struct contact* c, string path);
void contact_address_rev_load(struct contact* c, string path, unsigned int identity_id);

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

void contact_add_message(struct contact* c, struct message_meta *mm, bool prepend = false, bool lock_out = true);
void contact_remove_pending(struct contact* c);

bool contact_process_message(struct identity *i, struct contact *c, unsigned char *packet_buffer_packet, unsigned int packet_len);
void contact_incoming_packetsets_cleanup(struct contact* c);

string contact_get_chat(unsigned int identity_id, unsigned int contact_id, time_t from = 0, time_t to = 0);

string contact_interface(enum socket_interface_request_type sirt, vector<string>* request_path, vector<string>* request_params, char *post_content, unsigned int post_content_length, char** status_code);