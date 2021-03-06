#include "contact.h"

#include "util.h"
#include "address_factory.h"
#include "crypto.h"
#include "lossy_pipe.h"

#include <sstream>
#include <iostream>
#include <algorithm>

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

struct contact* contact_static_get_pending() {
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

	c->identity_key_id = 0;

	crypto_key_init(&c->pub_key);
	crypto_key_init(&c->session_key);
	crypto_key_init(&c->session_key_inc);
	mutex_init(&c->session_key_inc_lock);

	c->session_established = 0;

	c->address = "";
	c->address_rev = address_rev;
	c->address_rev_established = time(nullptr);
	c->verified = false;

	c->cs = new struct contact_stats();
	contact_stats_init(c->cs);

	mutex_init(&c->outgoing_messages_lock);
}

void contact_create(struct contact* c, string name, string pubkey, string address) {
	c->name = name;

	c->identity_key_id = 0;

	crypto_key_init(&c->pub_key);
	crypto_key_init(&c->session_key);
	crypto_key_init(&c->session_key_inc);
	mutex_init(&c->session_key_inc_lock);

	crypto_key_name_set(&c->pub_key, c->name.c_str(), c->name.length());
	c->pub_key.private_key_len = 0;
	c->pub_key.public_key = (char*)malloc(pubkey.length() + 1);
	memcpy(c->pub_key.public_key, pubkey.c_str(), pubkey.length());
	c->pub_key.public_key[pubkey.length()] = '\0';
	c->pub_key.public_key_len = pubkey.length() + 1;

	c->address = address;
	c->address_rev = "";
	c->address_rev_established = 0;

	c->session_established = 0;
	c->verified = false;

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

	contact_identity_key_id_save(c, path);

	if (c->pub_key.public_key_len > 0) {
		contact_pubkey_save(c, path);
	}

	contact_address_save(c, path);

	contact_address_rev_save(c, path);

	if (c->session_key.private_key_len > 0) {
		contact_sessionkey_save(c, path);
	}

	contact_verified_save(c, path);

	stringstream contact_stats_path;
	contact_stats_path << path << "contact_stats.bin";
	util_file_write_binary(contact_stats_path.str(), (unsigned char*)c->cs, sizeof(struct contact_stats));
}

void contact_load(struct contact* c, unsigned int identity_id, unsigned int id, string path) {
	c->id = id;

	stringstream name_path;
	name_path << path << "name";
	c->name = util_file_read_lines(name_path.str(), true)[0];

	contact_identity_key_id_load(c, path);

	crypto_key_init(&c->pub_key);
	contact_pubkey_load(c, path);

	contact_address_load(c, path);

	contact_address_rev_load(c, path, identity_id);

	crypto_key_init(&c->session_key);
	crypto_key_init(&c->session_key_inc);
	mutex_init(&c->session_key_inc_lock);
	contact_sessionkey_load(c, path);
	c->session_key.public_key_len = 0;
	crypto_key_name_set(&c->session_key, c->name.c_str(), c->name.length());
	crypto_key_copy(&c->session_key, &c->session_key_inc);

	contact_verified_load(c, path);

	contact_stats_load(c, path);

	mutex_init(&c->outgoing_messages_lock);
}

void contact_delete(struct contact* c, unsigned int identity_id) {
	stringstream path;
	path << "./identities/" << identity_id << "/contacts/" << c->id << "/";

	stringstream path_in;
	path_in << path.str() << "in/";
	vector<string> files_in = util_file_get_all_names(path_in.str(), 0, 0, false);
	for (int i = 0; i < files_in.size(); i++) {
		stringstream full_path;
		full_path << path_in.str() << files_in[i];

		util_file_delete(full_path.str());
	}
	util_directory_delete(path_in.str());

	stringstream path_out;
	path_out << path.str() << "out/";
	vector<string> files_out = util_file_get_all_names(path_out.str(), 0, 0, false);
	for (int i = 0; i < files_out.size(); i++) {
		stringstream full_path;
		full_path << path_out.str() << files_out[i];

		util_file_delete(full_path.str());
	}
	util_directory_delete(path_out.str());

	vector<string> files_base = util_file_get_all_names(path.str(), 0, 0, false);
	for (int i = 0; i < files_base.size(); i++) {
		stringstream full_path;
		full_path << path.str() << files_base[i];

		util_file_delete(full_path.str());
	}
	util_directory_delete(path.str());

	address_factory_remove_address(c->address_rev, AFST_CONTACT, identity_id, c->id);

	//TODO: memory cleanup
}

void contact_verified_save(struct contact* c, string path) {
	stringstream v_path;
	v_path << path << "verified";
	stringstream v_ss;
	v_ss << (c->verified == 1);
	util_file_write_line(v_path.str(), v_ss.str());
}

void contact_verified_load(struct contact* c, string path) {
	stringstream v_path;
	v_path << path << "verified";
	vector<string> v = util_file_read_lines(v_path.str(), true);
	if (v.size() > 0) {
		c->verified = (stoi(v[0].c_str()) == 1);
	}
	else {
		c->verified = false;
	}
}

void contact_identity_key_id_save(struct contact* c, unsigned int identity_id) {
	stringstream path;
	path << "./identities/" << identity_id << "/contacts/" << c->id << "/";
	contact_identity_key_id_save(c, path.str());
}

void contact_identity_key_id_save(struct contact* c, string path) {
	stringstream i_path;
	i_path << path << "identity_key_id";

	stringstream id;
	id << c->identity_key_id;

	util_file_write_line(i_path.str(), id.str());
}

void contact_identity_key_id_load(struct contact* c, string path) {
	stringstream i_path;
	i_path << path << "identity_key_id";

	vector<string> i_key = util_file_read_lines(i_path.str(), true);
	if (i_key.size() > 0) {
		c->identity_key_id = stoi(i_key[0]);
	}
	else {
		c->identity_key_id = 0;
	}
}

void contact_address_save(struct contact* c, unsigned int identity_id) {
	stringstream path;
	path << "./identities/" << identity_id << "/contacts/" << c->id << "/";
	contact_address_save(c, path.str());
}

void contact_address_save(struct contact* c, string path) {
	if (c->address.length() > 0) {
		stringstream add_path;
		add_path << path << "address";
		util_file_write_line(add_path.str(), c->address);
	}
}

void contact_address_load(struct contact* c, string path) {
	stringstream add_path;
	add_path << path << "address";
	vector<string> address = util_file_read_lines(add_path.str(), true);
	if (address.size() > 0) {
		c->address = address[0];
	}
	else {
		c->address = "";
	}
}

void contact_address_rev_save(struct contact* c, unsigned int identity_id) {
	stringstream path;
	path << "./identities/" << identity_id << "/contacts/" << c->id << "/";
	contact_address_rev_save(c, path.str());
}

void contact_address_rev_save(struct contact* c, string path) {
	if (c->address_rev.length() > 0) {
		stringstream add_r_path;
		add_r_path << path << "address_rev";
		vector<string> lines = vector<string>();
		lines.push_back(c->address_rev);

		stringstream established;
		established << c->address_rev_established;

		lines.push_back(established.str());
		util_file_write_lines(add_r_path.str(), lines);
	}
}

void contact_address_rev_load(struct contact* c, string path, unsigned int identity_id) {
	stringstream add_r_path;
	add_r_path << path << "address_rev";
	vector<string> address_rev = util_file_read_lines(add_r_path.str(), true);
	if (address_rev.size() > 0) {
		c->address_rev = address_rev[0];
		address_factory_add_address(address_rev[0], AFST_CONTACT, identity_id, c->id);
		if (address_rev.size() > 1 && address_rev[1].length() > 0) {
			c->address_rev_established = stoi(address_rev[1]);
		}
		else {
			c->address_rev_established = 0;
		}
	}
	else {
		c->address_rev = "";
	}
}

void contact_pubkey_load(struct contact* c, string path) {
	stringstream pk_path;
	pk_path << path << "pubkey";
	vector<string> pubkey = util_file_read_lines(pk_path.str(), true);

	crypto_key_init(&c->pub_key);
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
	}
	else {
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
	std::cout << c->identity_key_id << std::endl;
	crypto_key_dump(&c->session_key);
	crypto_key_dump(&c->session_key_inc);
}

void contact_add_message(struct contact* c, struct message_meta* mm, bool prepend, bool lock_out) {

	packetset_state_info psi;

	psi.hash_id = (unsigned char*)crypto_base64_encode(mm->msg_hash_id, 16);
	psi.identity_id = mm->identity_id;
	psi.contact_id = c->id;
	psi.ps = PS_OUT_CREATED;
	psi.mt = mm->mt;
	psi.info = nullptr;

	packetset_static_add_state_info(psi);


	if (lock_out) mutex_wait_for(&c->outgoing_messages_lock);
	if (prepend) {
		c->outgoing_messages.insert(c->outgoing_messages.begin(), mm);
	}
	else {
		c->outgoing_messages.push_back(mm);
	}

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

	if (lock_out) mutex_release(&c->outgoing_messages_lock);
}

void contact_remove_pending(struct contact* c) {
	mutex_wait_for(&contact_message_pending_lock);
	for (int mp = 0; mp < contact_message_pending.size(); mp++) {
		if (contact_message_pending[mp] == c) {
			contact_message_pending.erase(contact_message_pending.begin() + mp);
			break;
		}
	}
	mutex_release(&contact_message_pending_lock);
}

bool contact_process_message(struct identity* i, struct contact* c, unsigned char* packet_buffer_packet, unsigned int packet_len) {
	bool contact_established = false;
	bool session_key_established = false;
	bool symmetrically_decrypted = false;

	char* decrypted = nullptr;
	int out_len = 0;

	mutex_wait_for(&c->session_key_inc_lock);
	if (c->session_key_inc.private_key_len > 0 && c->session_key_inc.public_key_len == 0) {
		//SESSION KEY HAS BEEN ESTABLISHED
		session_key_established = true;
		decrypted = (char*)crypto_key_sym_decrypt(&c->session_key_inc, packet_buffer_packet, packet_len, &out_len);
		if (decrypted != nullptr) {
			symmetrically_decrypted = true;
		}
	}
	mutex_release(&c->session_key_inc_lock);

	if (decrypted == nullptr) {
		//EITHER ESTABLISH CONTACT OR ESTABLISH SESSION KEY
		int ikey_id = c->identity_key_id;
		for (; ikey_id < i->keys.size(); ikey_id++) {
			decrypted = crypto_key_private_decrypt(&i->keys[c->identity_key_id], (char*)packet_buffer_packet, packet_len, &out_len);
			if (decrypted != nullptr) {
				if (ikey_id > c->identity_key_id) {
					c->identity_key_id = ikey_id;
					std::cout << "migrated pubkey for contact: " << c->name << " by decryption" << std::endl;
					contact_identity_key_id_save(c, i->id);
				}
				break;
			}
		}
	}

	if (decrypted == nullptr) {
		std::cout << "decryption failed\n";
		contact_stats_update(c->cs, CSE_POLLUTION);
		return false;
	}

	struct NetworkPacket* np = new NetworkPacket();
	network_packet_create_from_data(np, decrypted, out_len);

	bool valid_outer_packet = false;

	int data_len = 0;
	char* data = nullptr;
	int hash_len = 0;
	char* hash_id = nullptr;
	int chunk_id = 0;
	int chunks_total = 0;
	int sent_ct = 0;

	bool verified = false;

	if (network_packet_read_str(np, &data, &data_len)) {
		if (network_packet_read_str(np, &hash_id, &hash_len)) {
			if (network_packet_read_int(np, &chunk_id)) {
				if (network_packet_read_int(np, &chunks_total)) {
					if (network_packet_read_int(np, &sent_ct)) {
						int np_pos_pre_sig = np->position;
						int sig_len = 0;
						char* signature = nullptr;
						if (network_packet_read_str(np, &signature, &sig_len)) {
							if (crypto_verify_signature(&c->pub_key, np->data, np_pos_pre_sig, signature, sig_len, false)) {
								verified = true;
							}
							else {
								std::cout << "message not verified" << std::endl;
							}
							free(signature);
							valid_outer_packet = true;
						}
					}
				}
			}
		}
	}

	network_packet_destroy(np);

	if (!valid_outer_packet) {
		if (data != nullptr) free(data);
		if (hash_id != nullptr) free(hash_id);
		contact_stats_update(c->cs, CSE_POLLUTION);
		return false;
	}

	struct packetset* ps = nullptr;

	map<string, struct packetset>::iterator ip = c->incoming_packetsets.find(string(hash_id));

	bool chunk_received = true;

	if (ip != c->incoming_packetsets.end()) {
		ps = &ip->second;
		if (ps->chunks_received_ct[chunk_id] == 0) {
			ps->chunks[chunk_id] = (unsigned char*)data;
			ps->chunks_length[chunk_id] = data_len;
			ps->verified[chunk_id] = verified;
		}
		else if (!ps->verified[chunk_id] && verified) {
			free(ps->chunks[chunk_id]);
			ps->chunks[chunk_id] = (unsigned char*)data;
			ps->chunks_length[chunk_id] = data_len;
			ps->verified[chunk_id] = verified;
		}
		else if (ps->verified[chunk_id] && !verified && symmetrically_decrypted) {
			//After PUBKEY_MIGRATION package resend of that MT_MIGRATE_PUBKEY package (, due to receipt packet loss,) is not verifiable.
			ps->chunks_received_ct[chunk_id]++;
			ps->transmission_last_receipt = time(nullptr);
			message_send_receipt(i, c, ps, hash_id, chunk_id);
			return true;
		}
		ps->chunks_received_ct[chunk_id]++;
		ps->transmission_last_receipt = time(nullptr);
	}
	else {
		struct packetset ps_ = packetset_create(chunks_total);
		ps_.chunks[chunk_id] = (unsigned char*)data;
		ps_.chunks_length[chunk_id] = data_len;
		ps_.chunks_received_ct[chunk_id] = 1;
		ps_.transmission_start = time(nullptr);
		ps_.transmission_last_receipt = time(nullptr);
		ps_.verified[chunk_id] = verified;
		c->incoming_packetsets.insert(pair<string, struct packetset>(string(hash_id), ps_));

		packetset_state_info psi;

		psi.hash_id = (unsigned char*)crypto_base64_encode((unsigned char*)hash_id, 16);
		psi.identity_id = i->id;
		psi.contact_id = c->id;
		psi.ps = PS_IN_PENDING;
		psi.mt = MT_UNKNOWN;
		psi.info = nullptr;

		packetset_static_add_state_info(psi);

		if (chunks_total == 1) {
			ip = c->incoming_packetsets.find(string(hash_id));
			ps = &ip->second;
		}
	}

	if (ps != nullptr) {
		bool complete = true;
		bool verified_c = verified;
		if (chunks_total > 1) {
			for (int ch = 0; ch < chunks_total; ch++) {
				if (ps->chunks_received_ct[ch] == 0) {
					complete = false;
					break;
				}
				if (!ps->verified[ch]) verified_c = false;
			}
			if (c->address.length() > 0) {
				if (verified) message_send_receipt(i, c, ps, hash_id, chunk_id);
			}
		}

		if (ps->processed && chunks_total == 1 && verified) {
			message_send_receipt(i, c, ps, hash_id, chunk_id);
		}

		if (complete && !ps->processed) {
			unsigned int complete_len = 0;
			unsigned char* complete_msg = packageset_message_get(ps, &complete_len);

			contact_stats_update(c->cs, CSE_TIME_PER_256_CHUNK_IN, (ps->transmission_last_receipt - ps->transmission_start) * 256.0f / complete_len);

			struct NetworkPacket* npc = new NetworkPacket();
			network_packet_create_from_data(npc, (char*)complete_msg, complete_len);

			int mt_int = 0;
			if (!network_packet_read_int(npc, &mt_int)) {
				free(hash_id);
				network_packet_destroy(npc);
				contact_stats_update(c->cs, CSE_MALFORMED);
				return false;
			}

			enum message_type mt = (enum message_type)mt_int;

			stringstream psi_info;

			if (verified_c == false) {
				if (mt == MT_ESTABLISH_CONTACT) {
					ps->processed = true;

					int name_len = 0;
					char* name = nullptr;
					int pubkey_len = 0;
					char* pubkey = nullptr;
					int address_rev_len = 0;
					char* address_rev = nullptr;

					bool valid_inner_packet = false;

					if (network_packet_read_str(npc, &name, &name_len)) {
						if (network_packet_read_str(npc, &pubkey, &pubkey_len)) {
							if (network_packet_read_str(npc, &address_rev, &address_rev_len)) {
								valid_inner_packet = true;
							}
						}
					}

					if (!valid_inner_packet) {
						if (name != nullptr) free(name);
						if (pubkey != nullptr) free(pubkey);
						if (address_rev != nullptr) free(address_rev);
						contact_stats_update(c->cs, CSE_MALFORMED);
					}
					else {
						if (c->pub_key.public_key_len == 0) {
							c->name = string(name);
							if (address_factory_check_multicast_address(string(address_rev))) {
								c->address = string(address_rev);
								contact_address_save(c, i->id);
							}
							crypto_key_init(&c->pub_key);
							c->pub_key.public_key = pubkey;
							c->pub_key.public_key_len = pubkey_len;
							contact_pubkey_save(c, i->id);
						}
						for (int ch = 0; ch < chunks_total; ch++) {
							message_send_receipt(i, c, ps, hash_id, ch);
						}
						free(name);
						free(address_rev);
					}
				}
				else {
					contact_stats_update(c->cs, CSE_POLLUTION);
				}
			}
			else if (verified_c) {
				ps->processed = true;
				if (mt == MT_ESTABLISH_SESSION) {
					int pubkey_len = 0;
					char* pubkey = nullptr;
					if (!network_packet_read_str(npc, &pubkey, &pubkey_len)) {
						contact_stats_update(c->cs, CSE_MALFORMED);
					}
					else {
						mutex_wait_for(&c->outgoing_messages_lock);

						if (c->session_key.private_key_len == 0) {
							message_send_session_key(i, c);
							if (pubkey_len == c->session_key.public_key_len) {
								memcpy(c->session_key.public_key, pubkey, pubkey_len);

								crypto_key_sym_finalise(&c->session_key);
								mutex_wait_for(&c->session_key_inc_lock);
								crypto_key_copy(&c->session_key, &c->session_key_inc);
								mutex_release(&c->session_key_inc_lock);
								c->session_established = time(nullptr);

								contact_sessionkey_save(c, i->id);
							}
						}
						else if (c->session_key.public_key_len > 0) {
							if (pubkey_len == c->session_key.public_key_len) {
								memcpy(c->session_key.public_key, pubkey, pubkey_len);

								crypto_key_sym_finalise(&c->session_key);
								mutex_wait_for(&c->session_key_inc_lock);
								crypto_key_copy(&c->session_key, &c->session_key_inc);
								mutex_release(&c->session_key_inc_lock);
								c->session_established = time(nullptr);

								contact_sessionkey_save(c, i->id);
							}
						}

						mutex_release(&c->outgoing_messages_lock);

						free(pubkey);
						if (chunks_total == 1) {
							message_send_receipt(i, c, ps, hash_id, chunk_id);
						}
					}
				}
				else if (mt == MT_MIGRATE_ADDRESS) {
					int address_len = 0;
					char* address = nullptr;
					if (!network_packet_read_str(npc, &address, &address_len)) {
						contact_stats_update(c->cs, CSE_MALFORMED);
					}
					else {

						mutex_wait_for(&c->outgoing_messages_lock);
						std::cout << "migrated address from: " << c->address << " to: " << address << std::endl;
						if (address_factory_check_multicast_address(string(address))) {
							c->address = string(address);
							contact_address_save(c, i->id);
						}
						mutex_release(&c->outgoing_messages_lock);

						free(address);

						if (chunks_total == 1) {
							message_send_receipt(i, c, ps, hash_id, chunk_id);
						}
					}
				}
				else if (mt == MT_MIGRATE_PUBKEY) {
					int pubkey_len = 0;
					char* pubkey = nullptr;
					if (!network_packet_read_str(npc, &pubkey, &pubkey_len)) {
						contact_stats_update(c->cs, CSE_MALFORMED);
					}
					else {
						mutex_wait_for(&c->outgoing_messages_lock);
						free(c->pub_key.public_key);
						c->pub_key.public_key_len = pubkey_len;
						c->pub_key.public_key = (char*)malloc(pubkey_len + 1);
						memcpy(c->pub_key.public_key, pubkey, pubkey_len);
						c->pub_key.public_key[pubkey_len] = '\0';
						crypto_key_reset_internal(&c->pub_key);
						mutex_release(&c->outgoing_messages_lock);

						contact_pubkey_save(c, i->id);

						free(pubkey);
						std::cout << "migrated pubkey:\n" << c->pub_key.public_key << std::endl;
					}
				}
				else if (mt == MT_DROP_SESSION) {

					mutex_wait_for(&c->session_key_inc_lock);
					c->session_key_inc.private_key_len = 0;
					if (c->session_key_inc.private_key != nullptr) {
						free(c->session_key_inc.private_key);
						c->session_key_inc.private_key = nullptr;
					}
					crypto_key_reset_internal(&c->session_key_inc, false);
					mutex_release(&c->session_key_inc_lock);

					mutex_wait_for(&c->outgoing_messages_lock);
					c->session_key.private_key_len = 0;
					if (c->session_key.private_key != nullptr) {
						free(c->session_key.private_key);
						c->session_key.private_key = nullptr;
					}
					crypto_key_reset_internal(&c->session_key, false);
					mutex_release(&c->outgoing_messages_lock);

					std::cout << "dropping session\n";
					if (chunks_total == 1) {
						message_send_receipt(i, c, ps, hash_id, chunk_id);
					}
				}
				else if (mt == MT_MESSAGE) {
					int message_len = 0;
					char* message = nullptr;
					if (!network_packet_read_str(npc, &message, &message_len)) {
						contact_stats_update(c->cs, CSE_MALFORMED);
					}
					else {
						std::cout << "\nMESSAGE: " << message << "\n";

						stringstream filename_base;
						filename_base << "./identities/" << i->id << "/contacts/" << c->id << "/in/";

						char* base64_packetset_hash = crypto_base64_encode((unsigned char*)hash_id, 16, true);
						string b64_ph(base64_packetset_hash);
						b64_ph = util_rtrim(b64_ph, "\r\n\t ");

						stringstream filename_l;
						do {
							filename_l.clear();
							filename_l << filename_base.str() << time(nullptr) << "." << b64_ph << ".message";
							util_sleep(1000);
						} while (util_path_exists(filename_l.str()));
						util_file_write_line(filename_l.str(), string(message));

						free(base64_packetset_hash);
						free(message);
						if (chunks_total == 1) {
							message_send_receipt(i, c, ps, hash_id, chunk_id);
						}
					}
				}
				else if (mt == MT_FILE) {
					int filename_len = 0;
					char* filename = nullptr;
					int message_len = 0;
					char* message = nullptr;

					if (!network_packet_read_str(npc, &filename, &filename_len) || !network_packet_read_str(npc, &message, &message_len)) {
						if (filename != nullptr) free(filename);
						contact_stats_update(c->cs, CSE_MALFORMED);
					}
					else {
						std::cout << "\nFile: complete " << message_len << std::endl;
						unsigned char* hash = crypto_hash_md5((unsigned char*)message, message_len);
						char* base_64 = crypto_base64_encode(hash, 16);
						std::cout << "base64-hash: " << base_64 << std::endl;
						free(hash);
						free(base_64);

						stringstream filename_base;
						filename_base << "./identities/" << i->id << "/contacts/" << c->id << "/in/";

						char* base64_packetset_hash = crypto_base64_encode((unsigned char*)hash_id, 16, true);
						string b64_ph(base64_packetset_hash);
						b64_ph = util_rtrim(b64_ph, "\r\n\t ");

						stringstream filename_l;
						do {
							filename_l.clear();
							filename_l << filename_base.str() << time(nullptr) << "." << b64_ph << ".file." << filename;
							util_sleep(1000);
						} while (util_path_exists(filename_l.str()));
						util_file_write_binary(filename_l.str(), (unsigned char*)message, message_len);

						free(base64_packetset_hash);
						free(filename);
						free(message);
						if (chunks_total == 1) {
							message_send_receipt(i, c, ps, hash_id, chunk_id);
						}
					}
				}
				else if (mt == MT_REQUEST_LOSSY_PIPE) {
					int descriptor_len = 0;
					char* descriptor = nullptr;
					int mtu = 0;
					if (!network_packet_read_str(npc, &descriptor, &descriptor_len) || !network_packet_read_int(npc, &mtu)) {
						if (descriptor != nullptr) free(descriptor);
					}
					else {
						packetset_state_info psi;

						psi_info << descriptor << ";" << mtu;

						std::cout << std::endl << "REQUEST_LOSSY_PIPE " << psi_info.str() << std::endl;

						free(descriptor);
						if (chunks_total == 1) {
							message_send_receipt(i, c, ps, hash_id, chunk_id);
						}
					}
				}
				else if (mt == MT_LOSSY_PIPE) {
					int descriptor_len = 0;
					char* descriptor = nullptr;
					int mtu = 0;
					int address_len = 0;
					char* address = nullptr;
					int port = 0;
					std::cout << std::endl << "LOSSY_PIPE" << std::endl;

					if (!network_packet_read_str(npc, &descriptor, &descriptor_len) || !network_packet_read_int(npc, &mtu) || !network_packet_read_str(npc, &address, &address_len) || !network_packet_read_int(npc, &port)) {
						if (descriptor != nullptr) free(descriptor);
						if (address != nullptr) free(address);
					}
					else {
						struct lossy_pipe* lp = new lossy_pipe();
						lossy_pipe_client_init(lp, descriptor, mtu, (unsigned char*)address, port, &c->session_key_inc);
						psi_info << lp->smb.name << ";" << lp->smb.size << ";" << lp->smb.slots << ";" << lp->smb.drop_skip;
						if (chunks_total == 1) {
							message_send_receipt(i, c, ps, hash_id, chunk_id);
						}
					}
				}
				else if (mt == MT_RECEIPT || mt == MT_RECEIPT_COMPLETE) {
					unsigned int receipt_of_chunk = 0;
					if (!network_packet_read_int(npc, (int*)&receipt_of_chunk)) {
						contact_stats_update(c->cs, CSE_MALFORMED);
					}
					else {
						mutex_wait_for(&c->outgoing_messages_lock);
						for (int pkts = 0; pkts < c->outgoing_messages.size(); pkts++) {
							bool same = true;
							for (int h = 0; h < 16; h++) {
								if (c->outgoing_messages[pkts]->msg_hash_id[h] != (unsigned char)hash_id[h]) {
									same = false;
									break;
								}
							}
							if (same) {
								struct packetset* pso = packetset_from_id(c->outgoing_messages[pkts]->packetset_id);
								if (mt == MT_RECEIPT_COMPLETE) {
									for (int ch = 0; ch < pso->chunks_ct; ch++) {
										if (pso->chunks_received_ct[ch] == 0) {
											pso->chunks_received_ct[ch]++;
										}
									}
									pso->mt_receipt_complete_received = true;
								}
								else {
									pso->chunks_received_ct[receipt_of_chunk]++;
								}
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
				}
			}
			if (ps->processed) {
				packetset_state_info psi;

				psi.hash_id = (unsigned char*)crypto_base64_encode((unsigned char*)hash_id, 16);
				psi.identity_id = i->id;
				psi.contact_id = c->id;
				psi.ps = PS_IN_COMPLETE;
				psi.mt = mt;

				if (psi_info.str().length() == 0) {
					psi.info = nullptr;
				}
				else {
					psi.info = (char*)malloc(psi_info.str().length() + 1);
					memcpy(psi.info, psi_info.str().c_str(), psi_info.str().length());
					psi.info[psi_info.str().length()] = '\0';
				}

				packetset_static_add_state_info(psi);
			}
			network_packet_destroy(npc);
		}
		if (chunk_received) {
			contact_stats_update(c->cs, CSE_CHUNK_RECEIVED);
			contact_stats_save(c, i->id);
		}

		contact_incoming_packetsets_cleanup(c);
	}
	free(hash_id);
	return true;
}

void contact_incoming_packetsets_cleanup(struct contact* c) {
	vector<string> remove = vector<string>();

	map<string, packetset>::iterator inc_ps = c->incoming_packetsets.begin();
	while (inc_ps != c->incoming_packetsets.end()) {
		if (inc_ps->second.processed && time(nullptr) - inc_ps->second.transmission_last_receipt > 60) {
			remove.push_back(inc_ps->first);
		}
		inc_ps++;
	}

	for (int r = 0; r < remove.size(); r++) {
		packetset_destroy(&(c->incoming_packetsets.find(remove[r])->second));
		c->incoming_packetsets.erase(remove[r]);
	}
}

string contact_get_chat(unsigned int identity_id, unsigned int contact_id, time_t from, time_t to) {
	struct identity* id = identity_get(identity_id);
	if (id == nullptr) return "{ }";
	struct contact* c = contact_get(&id->contacts, contact_id);
	if (c == nullptr) return "{ }";

	stringstream filename_base_in;
	filename_base_in << "./identities/" << id->id << "/contacts/" << c->id << "/in/";
	vector<string> filenames_in = util_file_get_all_names(filename_base_in.str(), from, to);
	for (int i = 0; i < filenames_in.size(); i++) {
		stringstream concat;
		concat << filenames_in[i] << ":" << filename_base_in.str();

		filenames_in[i] = concat.str();
	}

	stringstream filename_base_out;
	filename_base_out << "./identities/" << id->id << "/contacts/" << c->id << "/out/";
	vector<string> filenames_out = util_file_get_all_names(filename_base_out.str(), from, to);

	for (int i = 0; i < filenames_out.size(); i++) {
		stringstream concat;
		concat << filenames_out[i] << ":" << filename_base_out.str();

		filenames_out[i] = concat.str();
	}

	filenames_in.insert(filenames_in.end(), filenames_out.begin(), filenames_out.end());
	sort(filenames_in.begin(), filenames_in.end());

	stringstream result;
	result << "{\n";
	result << "\t\"identity_id\": " << identity_id << ",\n";
	result << "\t\"contact_id\": " << contact_id << ",\n";
	result << "\t\"chat\": {\n";

	for (int i = 0; i < filenames_in.size(); i++) {
		vector<string> splt = util_split(filenames_in[i], ":");

		vector<string> t_splt = util_split(splt[0], ".");

		time_t t = stoi(t_splt[0]);

		result << "\t\t\"" << t_splt[1] << "\": {\n";

		result << "\t\t\t\"time\": " << t << ",\n";
		result << "\t\t\t\"sender\": \"";

		if (strstr(splt[1].c_str(), "in") != nullptr) {
			result << c->name;
		}
		else {
			result << id->name;
		}

		result << "\",\n";

		const char* fn = splt[0].c_str();

		result << "\t\t\t\"pending\": ";
		if (strstr(fn, ".pending") == fn + splt[0].length() - strlen(".pending")) {
			result << 1;
		}
		else {
			result << 0;
		}
		result << ",\n";

		if (strstr(splt[0].c_str(), ".file.") != nullptr) {
			//is file
			result << "\t\t\t\"file\": \"";

			result << splt[1];

			for (int f = 0; f < t_splt.size(); f++) {
				result << t_splt[f];
				//std::cout << t_splt[f];
				if (f + 1 < t_splt.size()) {
					//std::cout << ".";
					result << ".";
				}
			}
			result << "\"\n";
		}
		else {
			stringstream fullpath;
			fullpath << splt[1] << splt[0];
			vector<string> lines = util_file_read_lines(fullpath.str());

			result << "\t\t\t\"message\": \"";
			for (int l = 0; l < lines.size(); l++) {
				if (l > 0) result << "\\n";
				result << lines[l];
			}
			result << "\"\n";
		}

		result << "\t\t}";
		if (i + 1 < filenames_in.size()) {
			result << ",";
		}
		result << "\n";
	}
	result << "\t}\n";
	result << "}\n";
	string res = result.str();
	return res;
}

string contact_interface(enum socket_interface_request_type sirt, vector<string>* request_path, vector<string>* request_params, char* post_content, unsigned int post_content_length, char** status_code) {
	string content = "{ }";
	const char* request_action = nullptr;
	if (request_path->size() > 1) {
		request_action = (*request_path)[1].c_str();

		if (sirt == SIRT_GET) {
			if (strstr(request_action, "chat") == request_action) {
				int identity_id = stoi(http_request_get_param(request_params, "identity_id"));
				int contact_id = stoi(http_request_get_param(request_params, "contact_id"));
				time_t time_start = stol(http_request_get_param(request_params, "time_start"));
				time_t time_end = stol(http_request_get_param(request_params, "time_end"));
				content = contact_get_chat(identity_id, contact_id, time_start, time_end);
				*status_code = (char*)HTTP_RESPONSE_200;
			}
			else {
				*status_code = (char*)HTTP_RESPONSE_404;
			}
		}
		else if (sirt == SIRT_POST) {
			if (strstr(request_action, "grant_permission") == request_action) {
				int identity_id = stoi(http_request_get_param(request_params, "identity_id"));
				int contact_id = stoi(http_request_get_param(request_params, "contact_id"));
				int message_type = stoi(http_request_get_param(request_params, "mt"));
				enum message_type mt = (enum message_type)message_type;
				if (mt == MT_REQUEST_LOSSY_PIPE) {
					string descriptor_mtu(post_content);
					content = message_send_lossy_pipe(identity_id, contact_id, descriptor_mtu);
				}
				*status_code = (char*)HTTP_RESPONSE_200;
			}
			else {
				*status_code = (char*)HTTP_RESPONSE_404;
			}
		}
		else if (sirt == SIRT_OPTIONS) {
			content = "Access-Control-Allow-Headers: Content-Type\n";
			*status_code = (char*)HTTP_RESPONSE_200;
		}
	}
	return content;
}