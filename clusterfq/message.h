#pragma once

#include "network.h"

#include <string>
#include <vector>
#include "socket_interface.h"

using namespace std;

enum message_type {
	MT_ESTABLISH_CONTACT,
	MT_ESTABLISH_SESSION,
	MT_MIGRATE_PUBKEY,
	MT_MIGRATE_ADDRESS,
	MT_DROP_SESSION,
	MT_MESSAGE,
	MT_FILE,
	MT_RECEIPT,
	MT_RECEIPT_COMPLETE,

	MT_REQUEST_LOSSY_PIPE,
	MT_LOSSY_PIPE,

	MT_UNKNOWN
};

struct message_receipt {
	string					address;
	unsigned char*			packet;
	unsigned int			packet_len;
};

struct message_meta {
	enum message_type		mt;
	
	unsigned int			identity_id;
	unsigned int			contact_id;

	struct NetworkPacket*	np;

	unsigned char*			msg_hash_id;

	unsigned int			packetset_id;

	time_t					time_pending;
};

void message_check_establish_contact(struct identity* i, struct contact* c);
void message_check_session_key(struct identity* i, struct contact* c);

bool message_check_pre(struct identity* i, struct contact* c);

void message_send_receipt(struct identity* i, struct contact* c, struct packetset* ps, char* hash_id, unsigned int chunk_id);
void message_send_migrate_key(struct identity* i, struct contact* c);
void message_send_session_key(struct identity* i, struct contact* c, bool prepend = false);
void message_send_new_address(struct identity* i, struct contact* c);

void message_resend_pending(struct identity* i, struct contact* c, enum message_type mt, time_t time_pending, string name, unsigned char* d, unsigned int d_len, unsigned char* hash_id);

void message_send(unsigned int identity_id, unsigned int contact_id, unsigned char* message, unsigned int msg_len);
//void message_send_file(unsigned int identity_id, unsigned int contact_id, unsigned char* path);
void message_send_file(unsigned int identity_id, unsigned int contact_id, string name, unsigned char* data, unsigned int data_len);
string message_delete(int identity_id, int contact_id, string hash_id, string sdir);

string message_send_lossy_pipe(unsigned int identity_id, unsigned int contact_id, string descriptor_mtu);
string message_request_lossy_pipe(unsigned int identity_id, unsigned int contact_id, string pipe_descriptor, int mtu);

string message_interface(enum socket_interface_request_type sirt, vector<string>* request_path, vector<string>* request_params, char *post_content, unsigned int post_content_length, char** status_code);