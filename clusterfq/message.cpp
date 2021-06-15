#include "message.h"

#include "identity.h"
#include "contact.h"
#include "client.h"
#include "address_factory.h"
#include "packetset.h"
#include "util.h"

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
	if (ps->complete || ps->processed) {
		network_packet_append_int(np, MT_RECEIPT_COMPLETE);
	} else {
		network_packet_append_int(np, MT_RECEIPT);
	}
	network_packet_append_int(np, chunk_id);

	struct NetworkPacket* outer = new NetworkPacket();
	network_packet_create(outer, 128 + 72 + 1000);
	network_packet_append_str(outer, np->data, np->position);
	network_packet_append_str(outer, hash_id, 16);
	network_packet_append_int(outer, 0); //chunk 0
	network_packet_append_int(outer, 1); //total chunks 1
	ps->chunks_sent_ct[chunk_id]++;
	network_packet_append_int(outer, ps->chunks_sent_ct[chunk_id]);

	unsigned int signature_len = 0;
	char* signature = crypto_sign_message(&i->keys[c->identity_key_id], outer->data, outer->position, &signature_len, false);
	network_packet_append_str(outer, signature, signature_len);
	free(signature);

	unsigned int out_size = 0;
	unsigned char* encrypted = nullptr;

	if (c->address_rev.length() == 0 || c->session_key.private_key_len == 0 || c->session_key.public_key_len > 0) {
		encrypted = (unsigned char*)crypto_key_public_encrypt(&c->pub_key, outer->data, outer->size, &out_size);
	} else {
		mutex_wait_for(&c->outgoing_messages_lock);
		encrypted = crypto_key_sym_encrypt(&c->session_key, (unsigned char *)outer->data, outer->size, (int *)&out_size);
		mutex_release(&c->outgoing_messages_lock);
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

	mutex_wait_for(&c->outgoing_messages_lock);
	mutex_wait_for(&c->session_key_inc_lock);
	crypto_key_sym_generate(&c->session_key);
	crypto_key_name_set(&c->session_key, c->name.c_str(), c->name.length());
	crypto_key_copy(&c->session_key, &c->session_key_inc);
	mutex_release(&c->session_key_inc_lock);
	mutex_release(&c->outgoing_messages_lock);

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

void message_send_new_address(struct identity* i, struct contact* c) {
	struct message_meta* mm = new struct message_meta();
	mm->mt = MT_MIGRATE_ADDRESS;
	mm->identity_id = i->id;
	mm->contact_id = c->id;
	mm->packetset_id = UINT_MAX;

	//GENERATE RETURN ADDRESS
	c->address_rev_migration = address_factory_get_unique();
	address_factory_add_address(c->address_rev_migration, AFST_CONTACT, i->id, c->id);

	//PACKET
							//type		//pubkey							//hash-id		//chunk-id		//resend-ct		//meta/pkt-overhead
	unsigned int size_est = 1 + c->address_rev_migration.length() + 17 + 6 + 10 + 40;

	struct NetworkPacket* np = new NetworkPacket();
	network_packet_create(np, size_est);
	network_packet_append_int(np, MT_MIGRATE_ADDRESS);
	network_packet_append_str(np, c->address_rev_migration.c_str(), c->address_rev_migration.length());

	mm->np = np;

	//MSG HASH-ID
	mm->msg_hash_id = message_create_hash_id(np->data, np->size);

	contact_add_message(c, mm);
}

void message_send_migrate_key(struct identity* i, struct contact* c) {
	message_check_session_key(i, c);

	struct message_meta* mm = new struct message_meta();
	mm->mt = MT_MIGRATE_PUBKEY;
	mm->identity_id = i->id;
	mm->contact_id = c->id;
	mm->packetset_id = UINT_MAX;

	//PACKET
							//type		//pubkey							//hash-id		//chunk-id		//resend-ct		//meta/pkt-overhead
	unsigned int size_est = 1 + i->keys[i->keys.size() - 1].public_key_len + 17 + 6 + 10 + 40;

	struct NetworkPacket* np = new NetworkPacket();
	network_packet_create(np, size_est);
	network_packet_append_int(np, MT_MIGRATE_PUBKEY);
	network_packet_append_str(np, i->keys[i->keys.size() - 1].public_key, i->keys[i->keys.size() - 1].public_key_len);

	mm->np = np;

	//MSG HASH-ID
	mm->msg_hash_id = message_create_hash_id(np->data, np->size);

	contact_add_message(c, mm);

	packetset_loop_start_if_needed();
}

void message_check_session_key(struct identity *i, struct contact *c) {
	if (c->session_key.private_key_len > 0 && time(nullptr) - c->session_established > 60) {

		mutex_wait_for(&c->session_key_inc_lock);
		c->session_key_inc.private_key_len = 0;
		if (c->session_key_inc.private_key != nullptr) {
			free(c->session_key_inc.private_key);
			c->session_key_inc.private_key = nullptr;
		}
		mutex_release(&c->session_key_inc_lock);

		mutex_wait_for(&c->outgoing_messages_lock);
		c->session_key.private_key_len = 0;
		if (c->session_key.private_key != nullptr) {
			free(c->session_key.private_key);
			c->session_key.private_key = nullptr;
		}
		mutex_release(&c->outgoing_messages_lock);

		//DROP SESSION
		struct message_meta* mm = new struct message_meta();
		mm->mt = MT_DROP_SESSION;
		mm->identity_id = i->id;
		mm->contact_id = c->id;
		mm->packetset_id = UINT_MAX;

		//PACKET
								//type	 //hash-id		//chunk-id		//resend-ct		//meta/pkt-overhead
		unsigned int size_est = 1 + 17 + 6 + 10 + 40;

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
}

void message_check_establish_contact(struct identity *i, struct contact *c) {
	if (c->address_rev.length() == 0) {
		//ESTABLISH CONTACT
		struct message_meta* mm = new struct message_meta();
		mm->mt = MT_ESTABLISH_CONTACT;
		mm->identity_id = i->id;
		mm->contact_id = c->id;
		mm->packetset_id = UINT_MAX;

		//GENERATE RETURN ADDRESS
		c->address_rev = address_factory_get_unique();
		address_factory_add_address(c->address_rev, AFST_CONTACT, i->id, c->id);
		c->address_rev_established = time(nullptr);

		//PACKET
								//type	  //name						//pubkey					//address							//hash-id		//chunk-id		//resend-ct		//meta/pkt-overhead
		unsigned int size_est = 1 + strlen(i->name.c_str()) + i->keys[0].public_key_len + strlen(c->address_rev.c_str()) + 17 + 6 + 10 + 40;

		struct NetworkPacket* np = new NetworkPacket();
		network_packet_create(np, size_est);
		network_packet_append_int(np, MT_ESTABLISH_CONTACT);
		network_packet_append_str(np, i->name.c_str(), i->name.length());
		network_packet_append_str(np, i->keys[i->keys.size() - 1].public_key, i->keys[i->keys.size() - 1].public_key_len);
		network_packet_append_str(np, c->address_rev.c_str(), strlen(c->address_rev.c_str()));

		mm->np = np;

		//MSG HASH-ID
		mm->msg_hash_id = message_create_hash_id(np->data, np->size);

		contact_add_message(c, mm);
	}
}

void message_check_pre(struct identity *i, struct contact *c) {
	unsigned int identity_id = i->id;
	if (c->address.length() == 0) {
		std::cout << "no address available" << std::endl;
		return;
	}

	if (c->pub_key.public_key_len == 0) {
		std::cout << "no pubkey available" << std::endl;
		return;
	}

	message_check_establish_contact(i, c);
	message_check_session_key(i, c);

	if (time(nullptr) - c->address_rev_established > 120) {
		message_send_new_address(i, c);
	}
}

void message_send(unsigned int identity_id, unsigned int contact_id, unsigned char *message, unsigned int msg_len) {
	struct identity* i = identity_get(identity_id);
	struct contact* c = contact_get(&i->contacts, contact_id);

	message_check_pre(i, c);

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

void message_send_file(unsigned int identity_id, unsigned int contact_id, unsigned char* path) {
	unsigned char* file = (unsigned char*)malloc(1024 * 1024 * 10);

	size_t out_len = 0;
	string filename((char*)path);
	util_file_read_binary(filename, file, &out_len);

	string name = util_file_get_name(filename);

	std::cout << "file input: " << out_len << std::endl;

	unsigned char* hash = crypto_hash_md5(file, out_len);
	char* base_64 = crypto_base64_encode(hash, 16);
	std::cout << "base64-hash: " << base_64 << std::endl;
	free(hash);
	free(base_64);

	struct identity* i = identity_get(identity_id);
	struct contact* c = contact_get(&i->contacts, contact_id);

	message_check_pre(i, c);

	struct message_meta* mm = new struct message_meta();
	mm->mt = MT_FILE;
	mm->identity_id = identity_id;
	mm->contact_id = contact_id;
	mm->packetset_id = UINT_MAX;

	//PACKET
							//type		//pubkey			//hash-id		//chunk-id		//resend-ct		//meta/pkt-overhead
	unsigned int size_est = 1 + out_len + 17 + 6 + 10 + 40;

	struct NetworkPacket* np = new NetworkPacket();
	network_packet_create(np, size_est);
	network_packet_append_int(np, MT_FILE);
	network_packet_append_str(np, name.c_str(), name.length());
	network_packet_append_str(np, (char*)file, out_len);

	free(file);

	mm->np = np;

	//MSG HASH-ID
	mm->msg_hash_id = message_create_hash_id(np->data, np->size);

	contact_add_message(c, mm);

	packetset_loop_start_if_needed();
}