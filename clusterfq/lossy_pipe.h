#pragma once

#include "network.h"
#include "shared_memory_buffer.h"
#include "thread.h"
#include "crypto.h"

#include <string>
#include <vector>

using namespace std;

struct lossy_pipe {
	string descriptor;
	int mtu;

	struct Network n;
	struct shared_memory_buffer smb;

	struct Key *k;
};

struct lossy_pipe_loop_params {
	struct lossy_pipe* lp;
	unsigned int	thread_id;
};

extern struct ThreadPool lossy_pipes_thread_pool;
extern vector<struct lossy_pipe*>	lossy_pipes;

void lossy_pipe_static_init();

bool lossy_pipe_init(struct lossy_pipe* lp, string descriptor, int mtu, unsigned char** out_address, int* out_port, struct Key* k);
bool lossy_pipe_client_init(struct lossy_pipe* lp, string descriptor, int mtu, unsigned char* address, int port, struct Key* k);

void lossy_pipe_loop(void* param);
void lossy_pipe_send_loop(void* param);