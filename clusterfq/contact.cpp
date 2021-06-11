#include "contact.h"

#include "util.h"
#include "address_factory.h"

#include <sstream>
#include <iostream>

#ifdef _WIN32
#else
#include <cstring>
#endif

vector<struct contact*>		contact_message_pending;
struct mutex				contact_message_pending_lock;
unsigned int				contact_message_pending_it = 0;

void contact_static_init() {
	mutex_init(&contact_message_pending_lock);
}

struct contact *contact_static_get_pending() {
	struct contact* c = nullptr;
	mutex_wait_for(&contact_message_pending_lock);
	if (contact_message_pending_it >= contact_message_pending.size()) {
		contact_message_pending_it = 0;
	}
	if (contact_message_pending_it < contact_message_pending.size()) {
		c = contact_message_pending[contact_message_pending_it];
	}
	contact_message_pending_it++;
	mutex_release(&contact_message_pending_lock);
	return c;
}

void contact_create_open(struct contact* c, string name_placeholder, string address_rev) {
	c->name = name_placeholder;

	c->pub_key.name_len = 0;
	c->pub_key.private_key_len = 0;
	c->pub_key.public_key_len = 0;

	c->session_key.private_key_len = 0;
	c->session_established = 0;
	
	c->address = "";
	c->address_rev = address_rev;

	c->cs = new struct contact_stats();
	contact_stats_init(c->cs);

	mutex_init(&c->outgoing_messages_lock);
}

void contact_create(struct contact* c, string name, string pubkey, string address) {
	c->name = name;
	
	crypto_key_name_set(&c->pub_key, c->name.c_str(), c->name.length());
	c->pub_key.private_key_len = 0;
	c->pub_key.public_key = (char*)malloc(pubkey.length() + 1);
	memcpy(c->pub_key.public_key, pubkey.c_str(), pubkey.length());
	c->pub_key.public_key[pubkey.length()] = '\0';
	c->pub_key.public_key_len = pubkey.length() + 1;
	
	c->address = address;
	c->address_rev = "";

	c->session_key.private_key_len = 0;
	c->session_established = 0;

	c->cs = new struct contact_stats();
	contact_stats_init(c->cs);

	mutex_init(&c->outgoing_messages_lock);
}

struct contact* contact_get(vector<struct contact>* contacts, unsigned int id) {
	for (int c = 0; c < contacts->size(); c++) {
		if ((*(contacts))[c].id == id) {
			return &(*(contacts))[c];
		}
	}
	return nullptr;
}

void contact_save(struct contact* c, string path) {
	stringstream name_path;
	name_path << path << "name";

	util_file_write_line(name_path.str(), c->name);

	if (c->pub_key.public_key_len > 0) {
		contact_pubkey_save(c, path);
	}

	if (c->address.length() > 0) {
		stringstream add_path;
		add_path << path << "address";
		util_file_write_line(add_path.str(), c->address);
	}

	if (c->address_rev.length() > 0) {
		stringstream add_r_path;
		add_r_path << path << "address_rev";
		util_file_write_line(add_r_path.str(), c->address_rev);
	}

	if (c->session_key.private_key_len > 0) {
		contact_sessionkey_save(c, path);
	}

	stringstream contact_stats_path;
	contact_stats_path << path << "contact_stats.bin";
	util_file_write_binary(contact_stats_path.str(), (unsigned char *)c->cs, sizeof(struct contact_stats));
}

void contact_load(struct contact* c, unsigned int identity_id, unsigned int id, string path) {
	c->id = id;

	stringstream name_path;
	name_path << path << "name";
	c->name = util_file_read_lines(name_path.str(), true)[0];
	
	contact_pubkey_load(c, path);

	stringstream add_path;
	add_path << path << "address";
	vector<string> address = util_file_read_lines(add_path.str(), true);
	if (address.size() > 0) {
		c->address = address[0];
	} else {
		c->address = "";
	}

	stringstream add_r_path;
	add_r_path << path << "address_rev";
	vector<string> address_rev = util_file_read_lines(add_r_path.str(), true);
	if (address_rev.size() > 0) {
		c->address_rev = address_rev[0];
		address_factory_add_address(address_rev[0], AFST_CONTACT, identity_id, c->id);
	} else {
		c->address_rev = "";
	}

	contact_sessionkey_load(c, path);
	c->session_key.public_key_len = 0;
	crypto_key_name_set(&c->session_key, c->name.c_str(), c->name.length());

	contact_stats_load(c, path);

	mutex_init(&c->outgoing_messages_lock);
}

void contact_pubkey_load(struct contact* c, string path) {
	stringstream pk_path;
	pk_path << path << "pubkey";
	vector<string> pubkey = util_file_read_lines(pk_path.str(), true);

	crypto_key_name_set(&c->pub_key, c->name.c_str(), c->name.length());
	c->pub_key.private_key_len = 0;
	c->pub_key.public_key_len = 0;

	if (pubkey.size() > 0) {
		stringstream prk;
		for (int pk = 0; pk < pubkey.size(); pk++) {
			prk << pubkey[pk] << "\n";
		}
		string prk_s = prk.str();
		const char* prk_c = prk_s.c_str();

		c->pub_key.public_key = (char*)malloc(prk.str().length() + 1);
		memcpy(c->pub_key.public_key, prk_c, prk.str().length());
		c->pub_key.public_key[prk.str().length()] = '\0';
		c->pub_key.public_key_len = prk.str().length() + 1;
	}
}

void contact_pubkey_save(struct contact* c, unsigned int identity_id) {
	stringstream path;
	path << "./identities/" << identity_id << "/contacts/" << c->id << "/";
	contact_pubkey_save(c, path.str());
}

void contact_pubkey_save(struct contact* c, string path) {
	stringstream pk_path;
	pk_path << path << "pubkey";
	util_file_write_line(pk_path.str(), c->pub_key.public_key);
}

void contact_sessionkey_load(struct contact* c, string path) {
	unsigned char buffer[1024];
	stringstream sk_path;
	sk_path << path << "sessionkey.bin";
	size_t out_len = 0;
	util_file_read_binary(sk_path.str(), (unsigned char*)&buffer, &out_len);
	stringstream sk_meta;
	sk_meta << path << "sessionkey.meta";
	vector<string> sessionkey_meta = util_file_read_lines(sk_meta.str(), true);
	if (sessionkey_meta.size() > 0) {
		vector<string> splt = util_split(sessionkey_meta[0], ";");
		c->session_established = stol(splt[0]);
		c->session_key.private_key = (char*)malloc(out_len + 1);
		memcpy(c->session_key.private_key, &buffer, out_len);
		c->session_key.private_key[out_len] = '\0';
		c->session_key.private_key_len = out_len;
	} else {
		c->session_key.private_key_len = 0;
	}
}

void contact_sessionkey_save(struct contact* c, string path) {
	stringstream sk_path;
	sk_path << path << "sessionkey.bin";
	util_file_write_binary(sk_path.str(), (unsigned char*)c->session_key.private_key, c->session_key.private_key_len);
	stringstream sk_meta;
	sk_meta << path << "sessionkey.meta";
	stringstream sk_meta_data;
	sk_meta_data << c->session_established << ";" << c->session_key.private_key_len;
	util_file_write_line(sk_meta.str(), sk_meta_data.str());
}

void contact_sessionkey_save(struct contact* c, unsigned int identity_id) {
	stringstream path;
	path << "./identities/" << identity_id << "/contacts/" << c->id << "/";
	contact_sessionkey_save(c, path.str());
}

void contact_stats_save(struct contact* c, unsigned int identity_id) {
	stringstream path;
	path << "./identities/" << identity_id << "/contacts/" << c->id << "/";
	contact_stats_save(c, path.str());
}

void contact_stats_save(struct contact* c, string path) {
	stringstream contact_stats_path;
	contact_stats_path << path << "contact_stats.bin";
	util_file_write_binary(contact_stats_path.str(), (unsigned char*)c->cs, sizeof(struct contact_stats));
}

void contact_stats_load(struct contact* c, string path) {
	c->cs = new struct contact_stats();
	stringstream contact_stats_path;
	contact_stats_path << path << "contact_stats.bin";
	size_t out_len_ = 0;
	util_file_read_binary(contact_stats_path.str(), (unsigned char*)c->cs, &out_len_);
	if (out_len_ == 0) {
		contact_stats_init(c->cs);
	}
}

void contact_dump(struct contact* c) {
	std::cout << c->id << std::endl;
	std::cout << c->name << std::endl;
	std::cout << c->address << std::endl;
	std::cout << c->address_rev << std::endl;
	crypto_key_dump(&c->pub_key);
	crypto_key_dump(&c->session_key);
}

void contact_add_message(struct contact* c, struct message_meta* mm, bool prepend) {
	mutex_wait_for(&c->outgoing_messages_lock);
	if (prepend) {
		c->outgoing_messages.insert(c->outgoing_messages.begin(), mm);
	} else {
		c->outgoing_messages.push_back(mm);
	}
	mutex_release(&c->outgoing_messages_lock);
	mutex_wait_for(&contact_message_pending_lock);
	bool found = false;
	for (int mp = 0; mp < contact_message_pending.size(); mp++) {
		if (contact_message_pending[mp] == c) {
			found = true;
			break;
		}
	}
	if (!found) {
		contact_message_pending.push_back(c);
	}
	mutex_release(&contact_message_pending_lock);
}

bool contact_process_message(struct identity* i, struct contact* c, unsigned char* packet_buffer_packet, unsigned int packet_len) {
	bool contact_established = false;
	bool session_key_established = false;

	char* decrypted = nullptr;
	int out_len = 0;

	if (c->session_key.private_key_len > 0 && c->session_key.public_key_len == 0) {
		//SESSION KEY HAS BEEN ESTABLISHED
		session_key_established = true;
		decrypted = (char *)crypto_key_sym_decrypt(&c->session_key, packet_buffer_packet, packet_len, &out_len);
	}
	if (decrypted == nullptr) {
		//EITHER ESTABLISH CONTACT OR ESTABLISH SESSION KEY
		decrypted = crypto_key_private_decrypt(&i->keys[0], (char*)packet_buffer_packet, packet_len, &out_len);
	}

	if (decrypted == nullptr) {
		std::cout << "decryption failed\n";
		contact_stats_update(c->cs, CSE_POLLUTION);
		return false;
	}

	struct NetworkPacket* np = new NetworkPacket();
	network_packet_create_from_data(np, decrypted, out_len);
	int data_len = 0;
	char* data = network_packet_read_str(np, &data_len);
	int hash_len = 0;
	char* hash_id = network_packet_read_str(np, &hash_len);
	int chunk_id = network_packet_read_int(np);
	int chunks_total = network_packet_read_int(np);
	int sent_ct = network_packet_read_int(np);
	network_packet_destroy(np);

	struct packetset* ps = nullptr;

	map<string, struct packetset>::iterator ip = c->incoming_packetsets.find(string(hash_id));

	bool chunk_received = true;

	if (ip != c->incoming_packetsets.end()) {
		ps = &ip->second;
		if (ps->chunks_received_ct[chunk_id] == 0) {
			ps->chunks[chunk_id] = (unsigned char*)data;
			ps->chunks_length[chunk_id] = data_len;
		}
		ps->chunks_received_ct[chunk_id]++;
		ps->transmission_last_receipt = time(nullptr);
	} else {
		struct packetset ps_ = packetset_create(chunks_total);
		ps_.chunks[chunk_id] = (unsigned char*)data;
		ps_.chunks_length[chunk_id] = data_len;
		ps_.chunks_received_ct[chunk_id] = 1;
		ps_.transmission_start = time(nullptr);
		ps_.transmission_last_receipt = time(nullptr);
		c->incoming_packetsets.insert(pair<string, struct packetset>(string(hash_id), ps_));
		if (chunks_total == 1) {
			ip = c->incoming_packetsets.find(string(hash_id));
			ps = &ip->second;
		}
	}

	if (ps != nullptr) {
		bool complete = true;
		if (chunks_total > 1) {
			for (int ch = 0; ch < chunks_total; ch++) {
				if (ps->chunks_received_ct[ch] == 0) {
					complete = false;
					break;
				}
			}
			if (c->address.length() > 0) {
				message_send_receipt(i, c, ps, hash_id, chunk_id);
			}
		}

		if (complete) {
			unsigned int complete_len = 0;
			unsigned char* complete_msg = packageset_message_get(ps, &complete_len);

			contact_stats_update(c->cs, CSE_TIME_PER_256_CHUNK_IN, (ps->transmission_last_receipt - ps->transmission_start) * 256.0f/complete_len);

			struct NetworkPacket* npc = new NetworkPacket();
			network_packet_create_from_data(npc, (char*)complete_msg, complete_len);

			enum message_type mt = (enum message_type)network_packet_read_int(npc);
			if (mt == MT_ESTABLISH_CONTACT) {
				int name_len = 0;
				char* name = network_packet_read_str(npc, &name_len);

				int pubkey_len = 0;
				char* pubkey = network_packet_read_str(npc, &pubkey_len);

				int address_rev_len = 0;
				char* address_rev = network_packet_read_str(npc, &address_rev_len);

				if (c->pub_key.public_key_len == 0) {
					c->name = string(name);
					c->address = string(address_rev);
					c->pub_key.public_key = pubkey;
					c->pub_key.public_key_len = pubkey_len;
					contact_pubkey_save(c, i->id);
				}
				for (int ch = 0; ch < chunks_total; ch++) {
					message_send_receipt(i, c, ps, hash_id, ch);
				}
			} else if (mt == MT_ESTABLISH_SESSION) {
				int pubkey_len = 0;
				char* pubkey = network_packet_read_str(npc, &pubkey_len);

				if (c->session_key.private_key_len == 0) {
					message_send_session_key(i, c);
					memcpy(c->session_key.public_key, pubkey, pubkey_len);
					crypto_key_sym_finalise(&c->session_key);
					c->session_established = time(nullptr);
					contact_sessionkey_save(c, i->id);
				} else if (c->session_key.public_key_len > 0){
					memcpy(c->session_key.public_key, pubkey, pubkey_len);
					crypto_key_sym_finalise(&c->session_key);
					c->session_established = time(nullptr);
					contact_sessionkey_save(c, i->id);
				}
				if (chunks_total == 1) {
					message_send_receipt(i, c, ps, hash_id, chunk_id);
				}
			} else if (mt == MT_DROP_SESSION) {
				c->session_key.private_key_len = 0;
				if (c->session_key.private_key != nullptr) {
					free(c->session_key.private_key);
					c->session_key.private_key = nullptr;
				}
				std::cout << "dropping session\n";
				if (chunks_total == 1) {
					message_send_receipt(i, c, ps, hash_id, chunk_id);
				}
			} else if (mt == MT_MESSAGE) {
				int message_len = 0;
				char* message = network_packet_read_str(npc, &message_len);
				std::cout << "\nMESSAGE: " << message << "\n";

				if (chunks_total == 1) {
					message_send_receipt(i, c, ps, hash_id, chunk_id);
				}
			} else if (mt == MT_RECEIPT) {
				unsigned int receipt_of_chunk = network_packet_read_int(npc);
				mutex_wait_for(&c->outgoing_messages_lock);
				//migrate
				// - packetset map to map<hash_id, struct packetset>
				//   remove packetsetid
				for (int pkts = 0; pkts < c->outgoing_messages.size(); pkts++) {
					bool same = true;
					for (int h = 0; h < 16; h++) {
						if (c->outgoing_messages[pkts]->msg_hash_id[h] != (unsigned char)hash_id[h]) {
							same = false;
							break;
						}
					}
					if (same) {
						struct packetset *pso = packetset_from_id(c->outgoing_messages[pkts]->packetset_id);
						if (pso->mm->mt == MT_DROP_SESSION) {
							c->session_key.private_key_len = 0;
							if (c->session_key.private_key != nullptr) {
								free(c->session_key.private_key);
								c->session_key.private_key = nullptr;
							}
							std::cout << "dropping session\n";
						}
						pso->chunks_received_ct[receipt_of_chunk]++;
						if (pso->transmission_first_receipt == 0) {
							pso->transmission_first_receipt = time(nullptr);
						}
						pso->transmission_last_receipt = time(nullptr);

						contact_stats_update(c->cs, CSE_RECEIPT_RECEIVED);
						chunk_received = false;

						break;
					}
				}
				packetset_destroy(ps);
				c->incoming_packetsets.erase(string(hash_id));

				mutex_release(&c->outgoing_messages_lock);
			}

			network_packet_destroy(npc);
		}
		if (chunk_received) {
			contact_stats_update(c->cs, CSE_CHUNK_RECEIVED);
			contact_stats_save(c, i->id);
		}
	}
	return true;
}