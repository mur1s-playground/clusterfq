#pragma once

#include <string>

#ifndef SERVER_MAX_PACKTE_SIZE
#define SERVER_MAX_PACKET_SIZE 1024 * 16
#define SERVER_PACKET_BUFFER_SLOTS 50
#endif

using namespace std;

void server_init();
void server_group_join(string address);
void server_group_leave(string address);

unsigned char* server_wait_for_next_read_slot(unsigned int interval_milliseconds);

void server_loop(void* param);