#pragma once

#include <string>
#include <time.h>

#include "message.h"
#include "crypto.h"

using namespace std;

struct packetset {
	unsigned int			chunk_size;
	unsigned int			chunks_ct;

	unsigned char**			chunks;
	unsigned int*			chunks_length;

	unsigned int*			chunks_sent_ct;
	unsigned int*			chunks_received_ct;

	time_t					transmission_start;
	time_t					transmission_latest_sent;
	time_t					transmission_first_receipt;
	time_t					transmission_last_receipt;

	bool					complete;

	struct message_meta*	mm;
};

void packetset_loop(void* unused);
void packetset_static_init();
void packetset_loop_start_if_needed();

struct packetset packetset_create(unsigned int chunks_ct);
unsigned int packetset_create(struct message_meta *mm);
struct packetset* packetset_from_id(unsigned int id);
unsigned int packetset_prepare_send(struct packetset* ps);
unsigned char* packageset_message_get(struct packetset* ps, unsigned int *out_len);
void packetset_destroy(struct packetset* ps);

void packetset_enqueue_receipt(struct message_receipt mr);