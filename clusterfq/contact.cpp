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

	c->address = "";
	c->address_rev = address_rev;

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
		stringstream pk_path;
		pk_path << path << "pubkey";
		util_file_write_line(pk_path.str(), c->pub_key.public_key);
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
		stringstream sk_path;
		sk_path << path << "sessionkey";
		util_file_write_line(sk_path.str(), c->session_key.private_key);
	}
}

void contact_load(struct contact* c, unsigned int identity_id, unsigned int id, string path) {
	c->id = id;

	stringstream name_path;
	name_path << path << "name";
	c->name = util_file_read_lines(name_path.str(), true)[0];
	
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

	stringstream sk_path;
	sk_path << path << "sessionkey";
	vector<string> sessionkey = util_file_read_lines(sk_path.str(), true);

	crypto_key_name_set(&c->session_key, c->name.c_str(), c->name.length());
	c->session_key.private_key_len = 0;
	c->session_key.public_key_len = 0;

	if (sessionkey.size() > 0) {
		stringstream prk;
		for (int pk = 0; pk < sessionkey.size(); pk++) {
			prk << sessionkey[pk] << "\n";
		}
		string prk_s = prk.str();
		const char* prk_c = prk_s.c_str();

		c->session_key.private_key = (char*)malloc(prk.str().length() + 1);
		memcpy(c->session_key.private_key, prk_c, prk.str().length());
		c->session_key.private_key[prk.str().length()] = '\0';
		c->session_key.private_key_len = prk.str().length() + 1;
	}

	mutex_init(&c->outgoing_messages_lock);
}

void contact_dump(struct contact* c) {
	std::cout << c->id << std::endl;
	std::cout << c->name << std::endl;
	std::cout << c->address << std::endl;
	std::cout << c->address_rev << std::endl;
	crypto_key_dump(&c->pub_key);
	crypto_key_dump(&c->session_key);
}

void contact_add_message(struct contact* c, struct message_meta* mm) {
	mutex_wait_for(&c->outgoing_messages_lock);
	c->outgoing_messages.push_back(mm);
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
		std::cout << "symmetric dec packet len: " << packet_len << "\n";
		decrypted = (char *)crypto_key_sym_decrypt(&c->session_key, packet_buffer_packet, packet_len, &out_len);
	}
	if (decrypted == nullptr) {
		//EITHER ESTABLISH CONTACT OR ESTABLISH SESSION KEY
		decrypted = crypto_key_private_decrypt(&i->keys[0], (char*)packet_buffer_packet, packet_len, &out_len);
	}

	if (decrypted == nullptr) {
		std::cout << "decryption failed\n";
		return false;
	}

	struct NetworkPacket* np = new NetworkPacket();
	network_packet_create_from_data(np, decrypted, out_len);
	int data_len = 0;
	char* data = network_packet_read_str(np, &data_len);
	int hash_len = 0;
	char* hash_id = network_packet_read_str(np, &hash_len);
	//				  std::cout << "hash_id: " << hash_id << std::endl;
	int chunk_id = network_packet_read_int(np);
	//                std::cout << "chunk_id: " << chunk_id << std::endl;
	int chunks_total = network_packet_read_int(np);
	//                std::cout << "chunks_total: " << chunks_total << std::endl;
	int sent_ct = network_packet_read_int(np);
	//                std::cout << "data_len: " << data_len << std::endl;
	//                std::cout << "data: " << data << std::endl;
	network_packet_destroy(np);

	struct packetset* ps = nullptr;

	map<string, struct packetset>::iterator ip = c->incoming_packetsets.find(string(hash_id));

	if (ip != c->incoming_packetsets.end()) {
		ps = &ip->second;
		if (ps->chunks_received_ct[chunk_id] == 0) {
			ps->chunks[chunk_id] = (unsigned char*)data;
			ps->chunks_length[chunk_id] = data_len;
		}
		ps->chunks_received_ct[chunk_id]++;
	} else {
		struct packetset ps_ = packetset_create(chunks_total);
		ps_.chunks[chunk_id] = (unsigned char*)data;
		ps_.chunks_length[chunk_id] = data_len;
		ps_.chunks_received_ct[chunk_id] = 1;
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

			struct NetworkPacket* npc = new NetworkPacket();
			network_packet_create_from_data(npc, (char*)complete_msg, complete_len);

			enum message_type mt = (enum message_type)network_packet_read_int(npc);
			if (mt == MT_ESTABLISH_CONTACT) {
				int name_len = 0;
				char* name = network_packet_read_str(npc, &name_len);
				std::cout << "complete msg" << std::endl;
				std::cout << "len: " << complete_len << std::endl;

				std::cout << name << std::endl;

				int pubkey_len = 0;
				char* pubkey = network_packet_read_str(npc, &pubkey_len);

				std::cout << pubkey << std::endl;

				int address_rev_len = 0;
				char* address_rev = network_packet_read_str(npc, &address_rev_len);

				std::cout << address_rev << std::endl;

				c->name = string(name);
				c->address = string(address_rev);
				c->pub_key.public_key = pubkey;
				c->pub_key.public_key_len = pubkey_len;
				for (int ch = 0; ch < chunks_total; ch++) {
					std::cout << "preparing receipt" << ch << "\n";
					message_send_receipt(i, c, ps, hash_id, ch);
				}
			} else if (mt == MT_ESTABLISH_SESSION) {
				int pubkey_len = 0;
				char* pubkey = network_packet_read_str(npc, &pubkey_len);

				if (c->session_key.private_key_len == 0) {
					message_send_session_key(i, c);
					memcpy(c->session_key.public_key, pubkey, pubkey_len);
					crypto_key_sym_finalise(&c->session_key);
				} else if (c->session_key.public_key_len > 0){
					memcpy(c->session_key.public_key, pubkey, pubkey_len);
					crypto_key_sym_finalise(&c->session_key);
				}
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
				std::cout << "npc: " << npc->data << std::endl;
				mutex_wait_for(&c->outgoing_messages_lock);
				//migrate
				// - packetset map to map<hash_id, struct packetset>
				//   remove packetsetid
				std::cout << "received MT_RECEIPT for" << receipt_of_chunk << "\n";
				for (int pkts = 0; pkts < c->outgoing_messages.size(); pkts++) {
					bool same = true;
					std::cout << "comparing :" << c->outgoing_messages[pkts]->msg_hash_id << " to: " << hash_id << "\n";
					for (int h = 0; h < 16; h++) {
						if (c->outgoing_messages[pkts]->msg_hash_id[h] != (unsigned char)hash_id[h]) {
							same = false;
							std::cout << c->outgoing_messages[pkts]->msg_hash_id[h] << ": " << (int)c->outgoing_messages[pkts]->msg_hash_id[h] << "<>" << (int)(unsigned char)hash_id[h]<<":"<< hash_id[h] << "not same\n";
							break;
						}
					}
					if (same) {
						struct packetset *pso = packetset_from_id(c->outgoing_messages[pkts]->packetset_id);
						pso->chunks_received_ct[receipt_of_chunk]++;
						std::cout << "received receipt for chunk: " << receipt_of_chunk << " " << hash_id << std::endl;
						break;
					}
				}
				packetset_destroy(ps);
				c->incoming_packetsets.erase(string(hash_id));

				mutex_release(&c->outgoing_messages_lock);
			}
			network_packet_destroy(npc);
		}
	}
	return true;
}