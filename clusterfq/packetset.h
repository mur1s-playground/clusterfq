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
	bool					processed;

	bool*					verified;

	bool					mt_receipt_complete_received;

	struct message_meta*	mm;
};

enum packetset_state {
	PS_OUT_CREATED,
	PS_OUT_PENDING,
	PS_OUT_COMPLETE,
	PS_IN_PENDING,
	PS_IN_COMPLETE
};

struct packetset_state_info {
	unsigned char *hash_id;
	unsigned int identity_id;
	unsigned int contact_id;

	enum packetset_state ps;

	enum message_type mt;

	char* info;
};

void packetset_static_add_state_info(struct packetset_state_info psi);
string packetset_static_get_state_infos();

void packetset_remove_pending(struct identity* i, struct contact* c, unsigned char* hash_id);

void packetset_loop(void* unused);
void packetset_static_init();
void packetset_loop_start_if_needed();

struct packetset packetset_create(unsigned int chunks_ct);
unsigned int packetset_create(struct message_meta *mm);
void packetset_destroy(struct packetset* ps);
void packetsets_erase_from_id(unsigned int id);
struct packetset* packetset_from_id(unsigned int id);
unsigned int packetset_prepare_send(struct packetset* ps);
unsigned char* packageset_message_get(struct packetset* ps, unsigned int *out_len);
void packetset_destroy(struct packetset* ps);

void packetset_enqueue_receipt(struct message_receipt mr);

string packetset_interface(enum socket_interface_request_type sirt, vector<string>* request_path, vector<string>* request_params, char *post_content, unsigned int post_content_length, char** status_code);