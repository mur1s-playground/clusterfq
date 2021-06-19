#pragma once

#include "network.h"

struct socket_interface {
	struct Network tcp_server;
};

void socket_interface_static_init(int port);
void socket_interface_listen_loop();