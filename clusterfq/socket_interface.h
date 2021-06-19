#pragma once

#include "network.h"
#include <vector>
#include <string>

struct socket_interface {
	struct Network tcp_server;
};

enum socket_interface_request_type {
	SIRT_GET,
	SIRT_POST,
	SIRT_OPTIONS
};

extern const char* HTTP_RESPONSE_200;
extern const char* HTTP_RESPONSE_404;
extern const char* HTTP_RESPONSE_501;

using namespace std;

string http_request_get_param(vector<string>* params, string param);

void socket_interface_static_init(int port);
void socket_interface_listen_loop();