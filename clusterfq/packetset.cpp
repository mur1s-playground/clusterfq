#include "packetset.h"

#include <map>
#include <sstream>

#include "identity.h"
#include "client.h"
#include "util.h"
#include "thread.h"
#include "clusterfq.h"

#ifdef _WIN32
#else
#include <cstring>
#include <math.h>
#endif

#include <iostream>

map<int, struct packetset>		packetsets					= map<int, struct packetset>();
bool							packetset_loop_running		= false;
int								packetset_loop_thread_id	= -1;
struct mutex					packetset_loop_lock;

vector<struct message_receipt>	packetset_receipts_out = vector<struct message_receipt>();

void packetset_static_init() {
	mutex_init(&packetset_loop_lock);
}

void packetset_loop(void* unused) {
	std::cout << "packetset_loop started\n";
	while (true) {
		mutex_wait_for(&packetset_loop_lock);
		std::cout << "receipts size: " << packetset_receipts_out.size() << "\n"; 
		for (int pro = 0 ; pro < packetset_receipts_out.size(); pro++) {
			struct message_receipt* mr = &packetset_receipts_out[pro];
			std::cout << "sending receipt:" << mr  << "\n";
			client_send_to(mr->address, mr->packet, mr->packet_len);
			free(mr->packet);
		}
		packetset_receipts_out.clear();

		struct contact *c = contact_static_get_pending();
		if (c == nullptr) {
			std::cout << "nothing pending\n";
			break;
		}

		mutex_release(&packetset_loop_lock);
		mutex_wait_for(&c->outgoing_messages_lock);
		if (c->outgoing_messages.size() > 0) {
			for (int om = 0; om < c->outgoing_messages.size(); om++) {
				std::cout << "something to send\n";
				struct message_meta* mm = c->outgoing_messages[om];
				if (mm->packetset_id == UINT_MAX) {
					packetset_create(mm);
				}
				struct packetset* ps = packetset_from_id(mm->packetset_id);
				unsigned int chunks_left = packetset_prepare_send(ps);
				std::cout << om << ":"<<chunks_left << " chunks left\n";
				if (chunks_left > 0) break;
			}
		}
		mutex_release(&c->outgoing_messages_lock);

		util_sleep(5000);
	}
	packetset_loop_running = false;
	thread_terminated(&main_thread_pool, packetset_loop_thread_id);
	std::cout << "packetset_loop finished\n";
	mutex_release(&packetset_loop_lock);
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

	unsigned int free_id = 0;
	while (true) {
		map<int, struct packetset>::iterator psi = packetsets.find(free_id);
		if (psi == packetsets.end()) {
			break;
		}
		free_id++;
	}
	mm->packetset_id = free_id;

	int chunk_size = 0;

	if (ps.mm->mt == MT_ESTABLISH_CONTACT || ps.mm->mt == MT_ESTABLISH_SESSION) {
		chunk_size = 128;
	} else if (ps.mm->mt == MT_MESSAGE) {
		chunk_size = 1024;

	}

	int chunks = (int)ceilf(ps.mm->np->position / (float)chunk_size);

	ps.chunk_size = chunk_size;
	ps.chunks_ct = chunks;

	ps.chunks = (unsigned char**)malloc(chunks * sizeof(unsigned char*));
	ps.chunks_length = (unsigned int*)malloc(chunks * sizeof(unsigned int));

	ps.chunks_received_ct = (unsigned int*)malloc(chunks * sizeof(unsigned int));
	memset(ps.chunks_received_ct, 0, chunks * sizeof(unsigned int));

	ps.chunks_sent_ct = (unsigned int*)malloc(chunks * sizeof(unsigned int));
	memset(ps.chunks_sent_ct, 0, chunks * sizeof(unsigned int));

	packetsets.insert(pair<int, struct packetset>(free_id, ps));
	
	return free_id;
}

struct packetset packetset_create(unsigned int chunks_ct) {
	struct packetset ps;
	ps.chunks_ct = chunks_ct;
	
	ps.chunks = (unsigned char**)malloc(chunks_ct * sizeof(unsigned char*));
	ps.chunks_length = (unsigned int*)malloc(chunks_ct * sizeof(unsigned int));

	ps.chunks_received_ct = (unsigned int*)malloc(chunks_ct * sizeof(unsigned int));
	memset(ps.chunks_received_ct, 0, chunks_ct * sizeof(unsigned int));
	
	ps.chunks_sent_ct = (unsigned int*)malloc(chunks_ct * sizeof(unsigned int));
	memset(ps.chunks_sent_ct, 0, chunks_ct * sizeof(unsigned int));

	return ps;
}

struct packetset* packetset_from_id(unsigned int id) {
	map<int, struct packetset>::iterator ps_id = packetsets.find(id);
	if (ps_id != packetsets.end()) {
		return &ps_id->second;
	}
	return nullptr;
}

unsigned int packetset_prepare_send(struct packetset* ps) {
	struct identity* i = identity_get(ps->mm->identity_id);
	struct contact* con = contact_get(&i->contacts, ps->mm->contact_id);

	unsigned int sent_ct = 0;

	int message_start = 0;
	for (int c = 0; c < ps->chunks_ct; c++) {
		if (ps->chunks_received_ct[c] > 0) {
			continue;
		}
		sent_ct++;
		if (ps->chunks_sent_ct[c] > 0) {
			free(ps->chunks[c]);
		}

		int message_len = ps->chunk_size;
		if (message_start + message_len > ps->mm->np->position) {
			message_len = ps->mm->np->position - message_start;
		}

		//-------------------//
		//TODO: ADD SIGNATURE//
		//-------------------//
		struct NetworkPacket np;
		network_packet_create(&np, ps->chunk_size + 72);
		network_packet_append_str(&np, &ps->mm->np->data[message_start], message_len);
		network_packet_append_str(&np, (char *)ps->mm->msg_hash_id, 16);
		network_packet_append_int(&np, c);
		network_packet_append_int(&np, ps->chunks_ct);
		network_packet_append_int(&np, ps->chunks_sent_ct[c] + 1);

		if (ps->mm->mt == MT_ESTABLISH_CONTACT || ps->mm->mt == MT_ESTABLISH_SESSION) {
			ps->chunks[c] = (unsigned char*)crypto_key_public_encrypt(&con->pub_key, np.data, np.position, &ps->chunks_length[c]);
		} else if (ps->mm->mt == MT_MESSAGE) {
			ps->chunks[c] = (unsigned char*)crypto_key_sym_encrypt(&con->session_key, (unsigned char *)np.data, np.position, (int *)&ps->chunks_length[c]);
			std::cout << "\nsymmetric enc length: " << (int*)&ps->chunks_length[c] << "\n";
		}
		client_send_to(con->address, ps->chunks[c], ps->chunks_length[c]);
		ps->chunks_sent_ct[c]++;

		message_start += ps->chunk_size;
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

void packetset_destroy(struct packetset* ps) {
	for (int i = 0; i < ps->chunks_ct; i++) {
		free(ps->chunks[i]);
	}
	free(ps->chunks_length);
	free(ps->chunks_received_ct);
	free(ps->chunks_sent_ct);
}