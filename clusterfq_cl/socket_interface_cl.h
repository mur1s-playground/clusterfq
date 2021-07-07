#pragma once

#include <string>

using namespace std;

extern string socket_interface_address;
extern int socket_interface_port;

string socket_interface_query(const char* module, const char* module_action, int paramset_id = -1);