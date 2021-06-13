#pragma once

#include "network.h"

#include <string>

using namespace std;

enum message_type {
	MT_ESTABLISH_CONTACT,
	MT_ESTABLISH_SESSION,
	MT_DROP_SESSION,
	MT_MESSAGE,
	MT_RECEIPT,
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
};

void message_send_receipt(struct identity* i, struct contact* c, struct packetset* ps, char* hash_id, unsigned int chunk_id);
void message_send_session_key(struct identity* i, struct contact* c, bool prepend = false);
void message_send(unsigned int identity_id, unsigned int contact_id, unsigned char* message, unsigned int msg_len);
void message_send_file(unsigned int identity_id, unsigned int contact_id, unsigned char* message, unsigned int msg_len);