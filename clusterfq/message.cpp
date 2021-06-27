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

	//mutex_wait_for(&c->outgoing_messages_lock);
	mutex_wait_for(&c->session_key_inc_lock);
	crypto_key_sym_generate(&c->session_key);
	crypto_key_name_set(&c->session_key, c->name.c_str(), c->name.length());
	crypto_key_copy(&c->session_key, &c->session_key_inc);
	mutex_release(&c->session_key_inc_lock);
	//mutex_release(&c->outgoing_messages_lock);

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

	contact_add_message(c, mm, prepend, false);
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
	if (!message_check_pre(i, c)) return;

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
	mutex_wait_for(&c->outgoing_messages_lock);
	if (c->session_key.private_key_len > 0 && c->session_key.public_key_len == 0 && time(nullptr) - c->session_established > 60) {

		mutex_wait_for(&c->session_key_inc_lock);
		c->session_key_inc.private_key_len = 0;
		if (c->session_key_inc.private_key != nullptr) {
			free(c->session_key_inc.private_key);
			c->session_key_inc.private_key = nullptr;
		}
		mutex_release(&c->session_key_inc_lock);

		c->session_key.private_key_len = 0;
		if (c->session_key.private_key != nullptr) {
			free(c->session_key.private_key);
			c->session_key.private_key = nullptr;
		}

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

		contact_add_message(c, mm, false, false);
	}

	if (c->session_key.private_key_len == 0) {
		//ESTABLISH SESSION KEY
		message_send_session_key(i, c);
	}
	mutex_release(&c->outgoing_messages_lock);
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

bool message_check_pre(struct identity *i, struct contact *c) {
	unsigned int identity_id = i->id;
	if (c->address.length() == 0) {
		std::cout << "no address available" << std::endl;
		return false;
	}

	if (c->pub_key.public_key_len == 0) {
		std::cout << "no pubkey available" << std::endl;
		return false;
	}

	message_check_establish_contact(i, c);
	message_check_session_key(i, c);

	if (time(nullptr) - c->address_rev_established > 120) {
		message_send_new_address(i, c);
	}
	return true;
}

void message_pending_save(struct message_meta* mm, string name, unsigned char* data, unsigned int data_len) {
	stringstream filename_base;
	filename_base << "./identities/" << mm->identity_id << "/contacts/" << mm->contact_id << "/out/";

	char* base64_packetset_hash = crypto_base64_encode((unsigned char*)mm->msg_hash_id, 16, true);
	string b64_ph(base64_packetset_hash);
	b64_ph = util_rtrim(b64_ph, "\r\n\t ");

	time_t time_pending;

	stringstream filename_l;
	do {
		time_pending = time(nullptr);

		filename_l.clear();
		filename_l << filename_base.str() << time_pending << "." << b64_ph << ".";
		if (mm->mt == MT_MESSAGE) {
			filename_l << "message";
		} else if (mm->mt == MT_FILE) {
			filename_l << "file." << name;
		}
		filename_l << ".pending";
		util_sleep(1000);
	} while (util_path_exists(filename_l.str()));

	if (mm->mt == MT_MESSAGE) {
		util_file_write_line(filename_l.str(), string((char*)data));
	} else if (mm->mt == MT_FILE){
		util_file_write_binary(filename_l.str(), data, data_len);
	}

	mm->time_pending = time_pending;

	free(base64_packetset_hash);
}

void message_resend_pending(struct identity *i, struct contact *c, enum message_type mt, time_t time_pending, string name, unsigned char *d, unsigned int d_len, unsigned char *hash_id) {
	if (!message_check_pre(i, c)) return;

	struct message_meta* mm = new struct message_meta();
	mm->mt = mt;
	mm->identity_id = i->id;
	mm->contact_id = c->id;
	mm->packetset_id = UINT_MAX;

	unsigned int size_est = 1 + d_len + 17 + 6 + 10 + 40;

	struct NetworkPacket* np = new NetworkPacket();
	network_packet_create(np, size_est);
	network_packet_append_int(np, mt);
	if (mt == MT_FILE) {
		network_packet_append_str(np, name.c_str(), name.length());
	}
	network_packet_append_str(np, (char *)d, d_len);

	free(d);
	
	mm->np = np;
	mm->msg_hash_id = hash_id;
	mm->time_pending = time_pending;

	contact_add_message(c, mm);

	packetset_loop_start_if_needed();
}

void message_send(unsigned int identity_id, unsigned int contact_id, unsigned char *message, unsigned int msg_len) {
	struct identity* i = identity_get(identity_id);
	if (i == nullptr) return;
	struct contact* c = contact_get(&i->contacts, contact_id);
	if (c == nullptr) return;

	if (!message_check_pre(i, c)) return;

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

	message_pending_save(mm, "", message, msg_len);

	contact_add_message(c, mm);

	packetset_loop_start_if_needed();
}

void message_send_file(unsigned int identity_id, unsigned int contact_id, string name, unsigned char* data, unsigned int data_len) {
	struct identity* i = identity_get(identity_id);
	if (i == nullptr) return;
	struct contact* c = contact_get(&i->contacts, contact_id);
	if (c == nullptr) return;

	if (!message_check_pre(i, c)) return;

	size_t out_len = 0;

	//insert linefeeds
	int needed_lfs = data_len / 64;

	unsigned char* buffer = (unsigned char*)malloc(data_len + needed_lfs + 1);
	int ct = 0;
	for (int i = 0; i < data_len; i++) {
		if (i / 64 == 0) {
			buffer[ct] = '\n';
			ct++;
		}
		buffer[ct] = data[i];
		ct++;
	}
	buffer[ct] = '\0';

	unsigned char *bfile = crypto_base64_decode((char *)buffer, &out_len);
	free(buffer);

	string fname = util_file_get_name(name);

	std::cout << "file input: " << data_len << std::endl;

	unsigned char* hash = crypto_hash_md5(data, data_len);
	char* base_64 = crypto_base64_encode(hash, 16);
	std::cout << "base64-hash: " << base_64 << std::endl;
	free(hash);
	free(base_64);

	struct message_meta* mm = new struct message_meta();
	mm->mt = MT_FILE;
	mm->identity_id = identity_id;
	mm->contact_id = contact_id;
	mm->packetset_id = UINT_MAX;

	//PACKET
							//type		//pubkey			//hash-id		//chunk-id		//resend-ct		//meta/pkt-overhead
	unsigned int size_est = 1 + data_len + 17 + 6 + 10 + 40;

	struct NetworkPacket* np = new NetworkPacket();
	network_packet_create(np, size_est);
	network_packet_append_int(np, MT_FILE);
	network_packet_append_str(np, fname.c_str(), fname.length());
	network_packet_append_str(np, (char*)bfile, out_len);

	mm->np = np;

	//MSG HASH-ID
	mm->msg_hash_id = message_create_hash_id(np->data, np->size);

	message_pending_save(mm, fname, bfile, out_len);

	free(bfile);

	contact_add_message(c, mm);

	packetset_loop_start_if_needed();
}

string message_delete(int identity_id, int contact_id, string hash_id, string sdir) {
	struct identity* i = identity_get(identity_id);
	if (i == nullptr) return "{ }";
	struct contact* c = contact_get(&i->contacts, contact_id);
	if (c == nullptr) return "{ }";

	if (hash_id.length() == 0) return "{ }";

	string subdir = "";
	if (strstr(sdir.c_str(), "in")) {
		subdir = "in/";
	} else {
		subdir = "out/";
	}

	stringstream base_path;
	base_path << "./identities/" << i->id << "/contacts/" << c->id << "/" << subdir << "/";

	vector<string> files = util_file_get_all_names(base_path.str(), 0, 0);
	for (int f = 0; f < files.size(); f++) {
		if (strstr(files[f].c_str(), hash_id.c_str()) != nullptr) {
			stringstream fullpath;
			fullpath << base_path.str() << files[f];

			util_file_delete(fullpath.str());

			size_t out_len = 0;
			stringstream ss;
			ss << hash_id << "\n";
			unsigned char* dec = crypto_base64_decode(ss.str().c_str(), &out_len, true);
			packetset_remove_pending(i, c, dec);
			free(dec);
		}
	}

	stringstream result;
	result << "{\n";
	result << "\t\"identity_id\": " << identity_id << ",\n";
	result << "\t\"contact_id\": " << contact_id << ",\n";
	result << "\t\"hash_id\": \"" << hash_id << "\"\n";
	result << "}\n";
	return result.str();
}

string message_interface(enum socket_interface_request_type sirt, vector<string>* request_path, vector<string>* request_params, char *post_content, unsigned int post_content_length, char** status_code) {
	string content = "{ }";
	const char* request_action = nullptr;
	if (request_path->size() > 1) {
		request_action = (*request_path)[1].c_str();

		if (sirt == SIRT_GET) {
			*status_code = (char*)HTTP_RESPONSE_404;
		} else if (sirt == SIRT_POST) {
			if (strstr(request_action, "send") == request_action) {
				int identity_id = stoi(http_request_get_param(request_params, "identity_id"));
				int contact_id = stoi(http_request_get_param(request_params, "contact_id"));
				string type = http_request_get_param(request_params, "type");
				if (strstr(type.c_str(), "text") != nullptr) {
					string message(post_content);
					message = util_trim(message, "\r\n\t ");
					message_send(identity_id, contact_id, (unsigned char *)message.c_str(), message.length());
				} else if (strstr(type.c_str(), "file") != nullptr) {
					string filename = http_request_get_param(request_params, "filename");
					message_send_file(identity_id, contact_id, filename, (unsigned char *)post_content, post_content_length);
				}
				
				*status_code = (char*)HTTP_RESPONSE_200;
			} else if (strstr(request_action, "delete") == request_action) {
				int identity_id = stoi(http_request_get_param(request_params, "identity_id"));
				int contact_id = stoi(http_request_get_param(request_params, "contact_id"));
				string hash_id = http_request_get_param(request_params, "hash_id");
				string sdir = http_request_get_param(request_params, "sdir");
				content = message_delete(identity_id, contact_id, hash_id, sdir);
				*status_code = (char*)HTTP_RESPONSE_200;
			} else {
				*status_code = (char*)HTTP_RESPONSE_404;
			}
		} else if (sirt == SIRT_OPTIONS) {
			content = "Access-Control-Allow-Headers: Content-Type\n";
			*status_code = (char*)HTTP_RESPONSE_200;
		}
	}
	return content;
}