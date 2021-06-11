#include "message.h"

#include "identity.h"
#include "contact.h"
#include "client.h"
#include "address_factory.h"
#include "packetset.h"

#include <sstream>
#include <iostream>

#ifdef _WIN32
#else
#include <cstring>
#endif

unsigned char *message_create_hash_id(const char* data, unsigned int data_len) {
	char* d = (char *)malloc(data_len + 128);
	memcpy(d, data, data_len);
	char* rnd = crypto_random(128);
	memcpy(&d[data_len], rnd, 128);
	free(rnd);

	unsigned char *hash = crypto_hash_md5((unsigned char*)d, data_len + 128);
	free(d);
	return hash;
}

void message_send_receipt(struct identity* i, struct contact* c, struct packetset* ps, char* hash_id, unsigned int chunk_id) {
	//PACKET
								//type	  //hash-id		//chunk-id x2		//resend-ct		//meta/pkt-overhead
	unsigned int size_est = 1 + 17 + 6 + 10 + 40;

	struct NetworkPacket* np = new NetworkPacket();
	network_packet_create(np, size_est);
	network_packet_append_int(np, MT_RECEIPT);
	network_packet_append_int(np, chunk_id);

	struct NetworkPacket* outer = new NetworkPacket();
	network_packet_create(outer, 128 + 72);
	network_packet_append_str(outer, np->data, np->position);
	network_packet_append_str(outer, hash_id, 16);
	network_packet_append_int(outer, 0); //chunk 0
	network_packet_append_int(outer, 1); //total chunks 1

	ps->chunks_sent_ct[chunk_id]++;
	network_packet_append_int(outer, ps->chunks_sent_ct[chunk_id]);

	unsigned int out_size = 0;
	unsigned char* encrypted = nullptr;

	if (c->address_rev.length() == 0 || c->session_key.private_key_len == 0 || c->session_key.public_key_len > 0) {
		encrypted = (unsigned char*)crypto_key_public_encrypt(&c->pub_key, outer->data, outer->size, &out_size);
	} else {
		encrypted = crypto_key_sym_encrypt(&c->session_key, (unsigned char *)outer->data, outer->size, (int *)&out_size);
	}

	network_packet_destroy(outer);
	network_packet_destroy(np);

	struct message_receipt mr;
	mr.address = c->address;
	mr.packet = encrypted;
	mr.packet_len = out_size;

	contact_stats_update(c->cs, CSE_RECEIPT_SENT);

	packetset_enqueue_receipt(mr);
}

void message_send_session_key(struct identity* i, struct contact* c, bool prepend) {
	//ESTABLISH SESSION KEY
	struct message_meta* mm = new struct message_meta();
	mm->mt = MT_ESTABLISH_SESSION;
	mm->identity_id = i->id;
	mm->contact_id = c->id;
	mm->packetset_id = UINT_MAX;

	crypto_key_sym_generate(&c->session_key);

	//PACKET
							//type		//pubkey							//hash-id		//chunk-id		//resend-ct		//meta/pkt-overhead
	unsigned int size_est = 1 + c->session_key.public_key_len + 17 + 6 + 10 + 40;

	struct NetworkPacket* np = new NetworkPacket();
	network_packet_create(np, size_est);
	network_packet_append_int(np, MT_ESTABLISH_SESSION);
	network_packet_append_str(np, c->session_key.public_key, c->session_key.public_key_len);

	mm->np = np;

	//MSG HASH-ID
	mm->msg_hash_id = message_create_hash_id(np->data, np->size);

	contact_add_message(c, mm, prepend);
}

void message_send(unsigned int identity_id, unsigned int contact_id, unsigned char *message, unsigned int msg_len) {
	struct identity* i = identity_get(identity_id);
	struct contact* c = contact_get(&i->contacts, contact_id);

	if (c->address.length() == 0) {
		std::cout << "no address available" << std::endl;
		return;
	}

	if (c->pub_key.public_key_len == 0) {
		std::cout << "no pubkey available" << std::endl;
		return;
	}
	
	if (c->address_rev.length() == 0) {
		//ESTABLISH CONTACT
		struct message_meta *mm = new struct message_meta();
		mm->mt = MT_ESTABLISH_CONTACT;
		mm->identity_id = identity_id;
		mm->contact_id = contact_id;
		mm->packetset_id = UINT_MAX;

		//GENERATE RETURN ADDRESS
		c->address_rev = address_factory_get_unique();
		address_factory_add_address(c->address_rev, AFST_CONTACT, identity_id, contact_id);

		//PACKET
								//type	  //name						//pubkey					//address							//hash-id		//chunk-id		//resend-ct		//meta/pkt-overhead
		unsigned int size_est = 1		+ strlen(i->name.c_str())	+	i->keys[0].public_key_len + strlen(c->address_rev.c_str())	+	17			+	6			+	10			+	40;

		struct NetworkPacket *np = new NetworkPacket();
		network_packet_create(np, size_est);
		network_packet_append_int(np, MT_ESTABLISH_CONTACT);
		network_packet_append_str(np, i->name.c_str(), i->name.length());
		network_packet_append_str(np, i->keys[0].public_key, i->keys[0].public_key_len);
		network_packet_append_str(np, c->address_rev.c_str(), strlen(c->address_rev.c_str()));
		
		mm->np = np;

		//MSG HASH-ID
		mm->msg_hash_id = message_create_hash_id(np->data, np->size);

		contact_add_message(c, mm);
	}
	if (c->session_key.private_key_len > 0 && time(nullptr) - c->session_established > 60) {
		//DROP SESSION
		struct message_meta* mm = new struct message_meta();
		mm->mt = MT_DROP_SESSION;
		mm->identity_id = identity_id;
		mm->contact_id = contact_id;
		mm->packetset_id = UINT_MAX;

		//PACKET
								//type	 //hash-id		//chunk-id		//resend-ct		//meta/pkt-overhead
		unsigned int size_est = 1		+ 17 + 6 + 10 + 40;

		struct NetworkPacket* np = new NetworkPacket();
		network_packet_create(np, size_est);
		network_packet_append_int(np, MT_DROP_SESSION);

		mm->np = np;

		//MSG HASH-ID
		mm->msg_hash_id = message_create_hash_id(np->data, np->size);

		contact_add_message(c, mm);
	}

	if (c->session_key.private_key_len == 0) {
		//ESTABLISH SESSION KEY
		message_send_session_key(i, c);
	}

	struct message_meta *mm = new struct message_meta();
	mm->mt = MT_MESSAGE;
	mm->identity_id = identity_id;
	mm->contact_id = contact_id;
	mm->packetset_id = UINT_MAX;

	//PACKET
							//type		//pubkey			//hash-id		//chunk-id		//resend-ct		//meta/pkt-overhead
	unsigned int size_est = 1		+	msg_len			+	17			+	6			+	10			+	40;

	struct NetworkPacket *np = new NetworkPacket();
	network_packet_create(np, size_est);
	network_packet_append_int(np, MT_MESSAGE);
	network_packet_append_str(np, (char *)message, msg_len);

	mm->np = np;
	
	//MSG HASH-ID
	mm->msg_hash_id = message_create_hash_id(np->data, np->size);

	contact_add_message(c, mm);

	packetset_loop_start_if_needed();
}