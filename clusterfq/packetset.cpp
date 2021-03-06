#include "packetset.h"

#include <map>
#include <sstream>

#include "identity.h"
#include "client.h"
#include "util.h"
#include "thread.h"
#include "clusterfq.h"
#include "address_factory.h"

#ifdef _WIN32
#else
#include <cstring>
#include <math.h>
#endif

#include <iostream>

map<int, struct packetset>		packetsets					= map<int, struct packetset>();
struct mutex					packetsets_lock;

bool							packetset_loop_running		= false;
int								packetset_loop_thread_id	= -1;
struct mutex					packetset_loop_lock;

int								packetset_limit_per_second	= 2;
int								packetset_counter			= 0;
time_t							packetset_time_1			= 0;
time_t							packetset_time_2			= 0;


vector<struct message_receipt>	packetset_receipts_out = vector<struct message_receipt>();

struct mutex						packetset_state_info_lock;
vector<struct packetset_state_info> packetset_state_info_ = vector<struct packetset_state_info>();

void packetset_static_init() {
	mutex_init(&packetsets_lock);
	mutex_init(&packetset_loop_lock);
	mutex_init(&packetset_state_info_lock);
}

void packetset_static_add_state_info(struct packetset_state_info psi) {
	mutex_wait_for(&packetset_state_info_lock);
	packetset_state_info_.push_back(psi);
	mutex_release(&packetset_state_info_lock);
}

string packetset_static_get_state_infos() {
	mutex_wait_for(&packetset_state_info_lock);
	stringstream result;
	result << "{\n";
	result << "\t\"packetset_state_infos\": [\n";
	for (int psi = 0; psi < packetset_state_info_.size(); psi++) {
		result << "\t\t{\n";
		result << "\t\t\"hash_id\": \"";
		for (int hl = 0; hl < strlen((char*)packetset_state_info_[psi].hash_id) - 1; hl++) {
			result << packetset_state_info_[psi].hash_id[hl];
		}
		result << "\",\n";
		free(packetset_state_info_[psi].hash_id);
		result << "\t\t\"identity_id\": " << packetset_state_info_[psi].identity_id << ",\n";
		result << "\t\t\"contact_id\": " << packetset_state_info_[psi].contact_id << ",\n";
		result << "\t\t\"packetset_state_info\": " << packetset_state_info_[psi].ps << ",\n";
		if (packetset_state_info_[psi].info != nullptr) {
			result << "\t\t\"info\": \"" << packetset_state_info_[psi].info << "\",\n";
			free(packetset_state_info_[psi].info);
		}
		result << "\t\t\"message_type\": " << packetset_state_info_[psi].mt << "\n";
		result << "\t\t}";
		if (psi + 1 < packetset_state_info_.size()) {
			result << ",";
		}
		result << "\n";
	}
	result << "\t]\n";
	result << "}\n";
	packetset_state_info_.clear();
	mutex_release(&packetset_state_info_lock);
	string res = result.str();
	return res;
}

void packetset_remove_pending(struct identity *i, struct contact *c, unsigned char *hash_id) {
	mutex_wait_for(&c->outgoing_messages_lock);
	for (int om = c->outgoing_messages.size() - 1; om >= 0; om--) {
		struct message_meta* mm = c->outgoing_messages[om];
		bool same = true;
		std::cout << "checking hash\n";
		for (int h = 0; h < 16; h++) {
			std::cout << mm->msg_hash_id[h] << " == " << hash_id[h] << " -> " << (mm->msg_hash_id[h] == hash_id[h]) << "\n";
			if (mm->msg_hash_id[h] != hash_id[h]) {
				same = false;
				break;
			}
		}
		std::cout << "same: " << same << "\n";
		if (same) {
			std::cout << "removing pending packetset\n\n\n";
			free(mm->msg_hash_id);

			struct packetset *ps = packetset_from_id(mm->packetset_id);
			if (ps != nullptr) {
				packetset_destroy(ps);
				packetsets_erase_from_id(mm->packetset_id);
			}
			c->outgoing_messages.erase(c->outgoing_messages.begin() + om);
		}
	}
	//TODO: check for after deletion unneccessary session/address migration ps
	mutex_release(&c->outgoing_messages_lock);
}

void packetset_packet_limiter() {
	packetset_counter++;
	if (packetset_counter > packetset_limit_per_second) {
		do {
			util_sleep(1000 / packetset_limit_per_second);
			packetset_time_2 = time(nullptr);
		} while (packetset_time_2 - packetset_time_1 < 1);
		packetset_time_1 = time(nullptr);
		packetset_counter = 0;
	}
}

void packetset_send_receipts(bool lock) {
	if (lock) mutex_wait_for(&packetset_loop_lock);
	for (int pro = 0; pro < packetset_receipts_out.size(); pro++) {
		struct message_receipt* mr = &packetset_receipts_out[pro];
		client_send_to(mr->address, mr->packet, mr->packet_len);
		free(mr->packet);
		packetset_packet_limiter();
	}
	packetset_receipts_out.clear();
	if (lock) mutex_release(&packetset_loop_lock);
}

void packetset_loop(void* unused) {
	std::cout << "packset_loop: start" << std::endl;
	while (true) {
		if (debug_toggle) std::cout << "packset_loop: begin_while_loop" << std::endl;
		mutex_wait_for(&packetset_loop_lock);
		packetset_send_receipts(false);

		struct contact *c = contact_static_get_pending();
		if (c == nullptr) {
			break;
		}

		if (debug_toggle) std::cout << "packset_loop: contact_id > "<< c->id << std::endl;

		mutex_release(&packetset_loop_lock);
		mutex_wait_for(&c->outgoing_messages_lock);
		if (c->outgoing_messages.size() > 0) {
			if (debug_toggle) std::cout << "packset_loop: outgoing_messages_size: " << c->outgoing_messages.size() << std::endl;
			for (int om = 0; om < c->outgoing_messages.size(); om++) {
				struct message_meta* mm = c->outgoing_messages[om];
				struct identity* i = identity_get(mm->identity_id);

				//prepend new session, if the session was dropped, while a message was in queue
				if (c->session_key.private_key_len == 0 && (mm->mt == MT_MESSAGE || mm->mt == MT_FILE || mm->mt == MT_REQUEST_LOSSY_PIPE || mm->mt == MT_LOSSY_PIPE)) {
					message_send_session_key(i, c, true);
					break;
				}

				if (mm->packetset_id == UINT_MAX) {
					packetset_create(mm);
				}
				struct packetset* ps = packetset_from_id(mm->packetset_id);

				if (!ps->complete) {
					if (ps->transmission_start == 0 || 
						(ps->transmission_start > 0 && time(nullptr) - ps->transmission_latest_sent >= 2)) {
						unsigned int chunks_sent = packetset_prepare_send(ps);
						if (chunks_sent > 0) {
							std::cout << "sent: " << om << ": " << chunks_sent << "/" << ps->chunks_ct << std::endl;

							packetset_state_info psi;

							psi.hash_id = (unsigned char*)crypto_base64_encode(ps->mm->msg_hash_id, 16);
							psi.identity_id = i->id;
							psi.contact_id = c->id;
							psi.ps = PS_OUT_PENDING;
							psi.mt = ps->mm->mt;
							psi.info = nullptr;

							packetset_static_add_state_info(psi);

							break;
						} else {
							if (!ps->mt_receipt_complete_received) {
								for (int ch = 0; ch < ps->chunks_ct; ch++) {
									ps->chunks_received_ct[ch] = 0;
								}
								break;
							}

							packetset_state_info psi;

							psi.hash_id = (unsigned char*)crypto_base64_encode((unsigned char *)ps->mm->msg_hash_id, 16);
							psi.identity_id = i->id;
							psi.contact_id = c->id;
							psi.ps = PS_OUT_COMPLETE;
							psi.mt = ps->mm->mt;
							psi.info = nullptr;

							packetset_static_add_state_info(psi);
						}

						ps->complete = true;
						std::cout << "sent:" << om << std::endl;
						std::cout << "transmission complete\n";

						if (ps->mm->mt == MT_ESTABLISH_CONTACT) {
							struct identity* i = identity_get(mm->identity_id);
							struct contact* c = contact_get(&i->contacts, mm->contact_id);

							contact_address_rev_save(c, i->id);
						} else if (ps->mm->mt == MT_MIGRATE_ADDRESS) {
							struct identity* i = identity_get(mm->identity_id);
							struct contact* c = contact_get(&i->contacts, mm->contact_id);

							std::cout << "migrated address_rev from: " << c->address_rev << " to: " << c->address_rev_migration << std::endl;

							address_factory_remove_address(c->address_rev, AFST_CONTACT, mm->identity_id, mm->contact_id);
							c->address_rev = c->address_rev_migration;
							c->address_rev_established = time(nullptr);

							c->address_rev_migration = "";

							contact_address_rev_save(c, i->id);
						} else if (ps->mm->mt == MT_MIGRATE_PUBKEY) {
							struct identity* i = identity_get(mm->identity_id);
							
							c->identity_key_id = i->keys.size() - 1;
							std::cout << "migrated pubkey for contact: " << c->name << " by receipts to id: " << c->identity_key_id <<std::endl;
							contact_identity_key_id_save(c, i->id);
						} else if (ps->mm->mt == MT_MESSAGE || ps->mm->mt == MT_FILE) {
							struct identity* i = identity_get(mm->identity_id);
							stringstream filename_base;
							filename_base << "./identities/" << i->id << "/contacts/" << c->id << "/out/";

							char* base64_packetset_hash = crypto_base64_encode((unsigned char*)ps->mm->msg_hash_id, 16, true);
							string b64_ph(base64_packetset_hash);
							b64_ph = util_rtrim(b64_ph, "\r\n\t ");

							stringstream fname_ss;
							if (ps->mm->mt == MT_FILE) {
								ps->mm->np->position = 0;
								int mt_int = 0;
								int out_len = 0;
								char* fname = nullptr;
								if (!network_packet_read_int(ps->mm->np, &mt_int) || !network_packet_read_str(ps->mm->np, &fname, &out_len)) {
									fname_ss << "unreadable_filename";
								} else {
									//TODO: sanity check on filename
									fname_ss << fname;
									free(fname);
								}
								ps->mm->np->position = ps->mm->np->size;
							}

							stringstream filename_pending;
							filename_pending << filename_base.str() << ps->mm->time_pending << "." << b64_ph << ".";

							stringstream filename_l;
							do {
								filename_l.clear();
								filename_l << filename_base.str() << time(nullptr) << "." << b64_ph << ".";
								if (ps->mm->mt == MT_MESSAGE) {
									filename_l << "message";
									filename_pending << "message";
								} else if (ps->mm->mt == MT_FILE) {
									filename_l << "file." << fname_ss.str();
									filename_pending << "file." << fname_ss.str();
								}
								util_sleep(1000);
							} while (util_path_exists(filename_l.str()));
							filename_pending << ".pending";
							util_file_rename(filename_pending.str(), filename_l.str());

							free(base64_packetset_hash);
						}
						
						free(mm->msg_hash_id);

						struct packetset* ps = packetset_from_id(mm->packetset_id);
						if (ps != nullptr) {
							packetset_destroy(ps);
							packetsets_erase_from_id(mm->packetset_id);
						}
						c->outgoing_messages.erase(c->outgoing_messages.begin() + om);
						break;
						
					}
					if (!ps->complete) break;
				}
			}
		} else {
			contact_remove_pending(c);
		}
		mutex_release(&c->outgoing_messages_lock);
		if (debug_toggle) std::cout << "packset_loop: end_while_loop" << std::endl;
		util_sleep(200);
	}
	packetset_loop_running = false;
	thread_terminated(&main_thread_pool, packetset_loop_thread_id);
	mutex_release(&packetset_loop_lock);
	std::cout << "packset_loop: end" << std::endl;
}

void packetset_loop_start_if_needed() {
	mutex_wait_for(&packetset_loop_lock);
	if (!packetset_loop_running) {
		packetset_loop_running = true;
		packetset_loop_thread_id = thread_create(&main_thread_pool, (void*)&packetset_loop, nullptr);
	}
	mutex_release(&packetset_loop_lock);
}

void packetset_enqueue_receipt(struct message_receipt mr) {
	mutex_wait_for(&packetset_loop_lock);
	packetset_receipts_out.push_back(mr);
	mutex_release(&packetset_loop_lock);
}

unsigned int packetset_create(struct message_meta* mm) {
	struct packetset ps;
	ps.mm = mm;

	int chunk_size = 0;

	if (ps.mm->mt == MT_ESTABLISH_CONTACT || ps.mm->mt == MT_ESTABLISH_SESSION || ps.mm->mt == MT_DROP_SESSION) {
		chunk_size = 128;
	} else if (ps.mm->mt == MT_MESSAGE || ps.mm->mt == MT_MIGRATE_ADDRESS || ps.mm->mt == MT_MIGRATE_PUBKEY || ps.mm->mt == MT_FILE || ps.mm->mt == MT_REQUEST_LOSSY_PIPE || ps.mm->mt == MT_LOSSY_PIPE) {
		chunk_size = 1024;
	}

	int chunks = (int)ceilf(ps.mm->np->position / (float)chunk_size);

	ps.chunk_size = chunk_size;
	ps.chunks_ct = chunks;

	ps.chunks = (unsigned char**)malloc(chunks * sizeof(unsigned char*));
	for (int ch = 0; ch < chunks; ch++) {
		ps.chunks[ch] = nullptr;
	}
	ps.chunks_length = (unsigned int*)malloc(chunks * sizeof(unsigned int));

	ps.chunks_received_ct = (unsigned int*)malloc(chunks * sizeof(unsigned int));
	memset(ps.chunks_received_ct, 0, chunks * sizeof(unsigned int));

	ps.chunks_sent_ct = (unsigned int*)malloc(chunks * sizeof(unsigned int));
	memset(ps.chunks_sent_ct, 0, chunks * sizeof(unsigned int));

	ps.transmission_start = 0;
	ps.transmission_latest_sent = 0;
	ps.transmission_first_receipt = 0;
	ps.transmission_last_receipt = 0;

	ps.complete = false;
	ps.processed = false;

	ps.verified = (bool*)malloc(chunks * sizeof(bool));
	memset(ps.verified, 0, chunks * sizeof(bool));

	ps.mt_receipt_complete_received = false;

	unsigned int free_id = 0;
	mutex_wait_for(&packetsets_lock);
	while (true) {
		map<int, struct packetset>::iterator ps_id = packetsets.find(free_id);
		if (ps_id == packetsets.end()) {
			mm->packetset_id = free_id;
			packetsets.insert(pair<int, struct packetset>(free_id, ps));
			break;
		}
		free_id++;
	}
	mutex_release(&packetsets_lock);
	return free_id;
}

struct packetset packetset_create(unsigned int chunks_ct) {
	struct packetset ps;
	ps.chunks_ct = chunks_ct;
	
	ps.chunks = (unsigned char**)malloc(chunks_ct * sizeof(unsigned char*));
	for (int ch = 0; ch < chunks_ct; ch++) {
		ps.chunks[ch] = nullptr;
	}
	ps.chunks_length = (unsigned int*)malloc(chunks_ct * sizeof(unsigned int));

	ps.chunks_received_ct = (unsigned int*)malloc(chunks_ct * sizeof(unsigned int));
	memset(ps.chunks_received_ct, 0, chunks_ct * sizeof(unsigned int));
	
	ps.chunks_sent_ct = (unsigned int*)malloc(chunks_ct * sizeof(unsigned int));
	memset(ps.chunks_sent_ct, 0, chunks_ct * sizeof(unsigned int));

	ps.transmission_start = 0;
	ps.transmission_latest_sent = 0;
	ps.transmission_first_receipt = 0;
	ps.transmission_last_receipt = 0;

	ps.verified = (bool*)malloc(chunks_ct * sizeof(bool));
	memset(ps.verified, 0, chunks_ct * sizeof(bool));

	ps.complete = false;
	ps.processed = false;

	ps.mt_receipt_complete_received = false;

	return ps;
}

void packetset_destroy(struct packetset *ps) {
	for (int ch = 0; ch < ps->chunks_ct; ch++) {
		if (ps->chunks[ch] != nullptr) free(ps->chunks[ch]);
	}
	free(ps->chunks);
	free(ps->chunks_length);
	free(ps->chunks_received_ct);
	free(ps->chunks_sent_ct);
	free(ps->verified);
}

struct packetset* packetset_from_id(unsigned int id) {
	mutex_wait_for(&packetsets_lock);
	map<int, struct packetset>::iterator ps_id = packetsets.find(id);
	if (ps_id != packetsets.end()) {
		struct packetset* ps = &ps_id->second;
		mutex_release(&packetsets_lock);
		return ps;
	}
	mutex_release(&packetsets_lock);
	return nullptr;
}

void packetsets_erase_from_id(unsigned int id) {
	mutex_wait_for(&packetsets_lock);
	packetsets.erase(id);
	mutex_release(&packetsets_lock);
}

/*/
void packetsets_insert(unsigned int id, struct packetset ps) {
	mutex_wait_for(&packetsets_lock);
	packetsets.insert(std::pair<int, packetset>(id, ps));
	mutex_release(&packetsets_lock);
}
*/

unsigned int packetset_prepare_send(struct packetset* ps) {
	struct identity* i = identity_get(ps->mm->identity_id);
	struct contact* con = contact_get(&i->contacts, ps->mm->contact_id);

	unsigned int sent_ct = 0;

	int message_start = 0;
	int receipt_counter = 0;
	for (int c = 0; c < ps->chunks_ct; c++) {
		if (receipt_counter % 5 == 0) {
			packetset_send_receipts(true);
		}
		receipt_counter++;
		if (ps->chunks_received_ct[c] > 0) {
			message_start += ps->chunk_size;
			continue;
		}
		sent_ct++;
		contact_stats_update(con->cs, CSE_CHUNK_SENT);
		
		if (ps->chunks_sent_ct[c] > 0) {
			free(ps->chunks[c]);
		} else {
			if (ps->transmission_start == 0) {
				ps->transmission_start = time(nullptr);
			}
		}

		int message_len = ps->chunk_size;
		if (message_start + message_len > ps->mm->np->position) {
			message_len = ps->mm->np->position - message_start;
		}

		struct NetworkPacket np;
		network_packet_create(&np, ps->chunk_size + 72 + 1000);
		network_packet_append_str(&np, &ps->mm->np->data[message_start], message_len);
		network_packet_append_str(&np, (char *)ps->mm->msg_hash_id, 16);
		network_packet_append_int(&np, c);
		network_packet_append_int(&np, ps->chunks_ct);
		network_packet_append_int(&np, ps->chunks_sent_ct[c] + 1);
		unsigned int signature_len = 0;
		char* signature = crypto_sign_message(&i->keys[con->identity_key_id], np.data, np.position, &signature_len, false);
		network_packet_append_str(&np, signature, signature_len);
		free(signature);

		if (ps->mm->mt == MT_ESTABLISH_CONTACT || ps->mm->mt == MT_ESTABLISH_SESSION || ps->mm->mt == MT_DROP_SESSION) {
			ps->chunks[c] = (unsigned char*)crypto_key_public_encrypt(&con->pub_key, np.data, np.position, &ps->chunks_length[c]);
		} else if (ps->mm->mt == MT_MESSAGE || ps->mm->mt == MT_MIGRATE_ADDRESS || ps->mm->mt == MT_MIGRATE_PUBKEY || ps->mm->mt == MT_FILE || ps->mm->mt == MT_REQUEST_LOSSY_PIPE || ps->mm->mt == MT_LOSSY_PIPE) {
			if (con->session_key.private_key_len > 0 && con->session_key.public_key_len == 0) {
				ps->chunks[c] = (unsigned char*)crypto_key_sym_encrypt(&con->session_key, (unsigned char*)np.data, np.position, (int*)&ps->chunks_length[c]);
			} else {
				network_packet_destroy(&np);
				std::cout << "no key\n";
				break;
			}
		}
		network_packet_destroy(&np);

		client_send_to(con->address, ps->chunks[c], ps->chunks_length[c]);
		packetset_packet_limiter();
		ps->chunks_sent_ct[c]++;
		ps->transmission_latest_sent = time(nullptr);

		message_start += ps->chunk_size;
		if (ps->mm->mt != MT_ESTABLISH_CONTACT) {
			break;
		}
	}
	if (sent_ct == 0) {
		int total_len = 0;
		for (int ch = 0; ch < ps->chunks_ct; ch++) {
			total_len += ps->chunks_length[ch];
		}
		contact_stats_update(con->cs, CSE_TIME_PER_256_CHUNK_OUT, (ps->transmission_last_receipt - ps->transmission_start) * 256.0f/total_len);
	}
	return sent_ct;
}

unsigned char* packageset_message_get(struct packetset* ps, unsigned int *out_len) {
	int total_len = 0;
	for (int c = 0; c < ps->chunks_ct; c++) {
		total_len += ps->chunks_length[c];
	}
	unsigned char* msg = (unsigned char*) malloc(total_len + 1);
	int msg_pos = 0;
	for (int c = 0; c < ps->chunks_ct; c++) {
		memcpy(&msg[msg_pos], ps->chunks[c], ps->chunks_length[c]);
		msg_pos += ps->chunks_length[c];
		
	}
	msg[total_len] = '\0';
	*out_len = total_len;
	return msg;
}

string packetset_interface(enum socket_interface_request_type sirt, vector<string>* request_path, vector<string>* request_params, char *post_content, unsigned int post_content_length, char** status_code) {
	string content = "{ }";
	const char* request_action = nullptr;
	if (request_path->size() > 1) {
		request_action = (*request_path)[1].c_str();
		
		if (sirt == SIRT_GET) {
			if (strstr(request_action, "poll") == request_action) {
				content = packetset_static_get_state_infos();
				*status_code = (char*)HTTP_RESPONSE_200;
			} else {
				*status_code = (char*)HTTP_RESPONSE_404;
			}
		} else if (sirt == SIRT_POST) {
			*status_code = (char*)HTTP_RESPONSE_404;
		} else if (sirt == SIRT_OPTIONS) {
			content = "Access-Control-Allow-Headers: Content-Type\n";
			*status_code = (char*)HTTP_RESPONSE_200;
		}
	}
	return content;
}