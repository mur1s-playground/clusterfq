#pragma once

#include <string>

#include "network.h"

using namespace std;

struct network_ref {
	struct Network	n;
	unsigned int	last_used;
};

void client_init();
void client_send_to(string address, unsigned char* packet_buffer, unsigned int packet_length);