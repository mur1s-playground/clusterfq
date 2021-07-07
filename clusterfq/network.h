#ifndef _NETWORK_H_
#define _NETWORK_H_

#ifdef _WIN32
#include <ws2tcpip.h>
#include <winsock2.h>
#include <MSWSock.h>
#include <io.h>
#include <windows.h>
#else
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <unistd.h>
#endif

enum NetworkState {
	NS_CREATED,
	NS_BOUND,
	NS_LISTENING,
	NS_CONNECTED,
	NS_ERROR,
	NS_NULL
};

struct Network {
	int socket;
#ifdef _WIN32
	ADDRINFO* ip6addr;

	LPFN_WSARECVMSG WSARecvMsg;
	DWORD			lpcbBytesReturned;
#else
	struct sockaddr_in6 ip6addr;
#endif
	enum NetworkState state;
	void (*send)(struct Network* network, const void* packet, size_t size);
	void (*read)(struct Network* network, void* packet, size_t size, char* dst_address, unsigned int* out_len);
};

struct NetworkPacket {
	char* data;
	size_t size;
	size_t max_size;
	size_t position;
};

extern void network_init(struct Network* network);
extern void network_destroy(struct Network* network);

void network_socket_send(struct Network* network, const void* packet, size_t size);
void network_socket_read(struct Network* network, void* packet, size_t size, char* dst_address = nullptr, unsigned int* out_len = nullptr);

/* PACKET */

extern void network_packet_create(struct NetworkPacket* packet, size_t max_size);
extern void network_packet_create_from_data(struct NetworkPacket* packet, char* data, int size);
extern void network_packet_append_str(struct NetworkPacket* packet, const char* str, int size);
extern void network_packet_append_int(struct NetworkPacket* packet, int num);
extern void network_packet_append_longlong(struct NetworkPacket* packet, long long num);

extern bool network_packet_read_str(struct NetworkPacket* packet, char** out, int* out_len);
extern bool network_packet_read_int(struct NetworkPacket* packet, int* out);
extern bool network_packet_read_longlong(struct NetworkPacket* packet, long long* out);

/*
extern char* network_packet_read_str(struct NetworkPacket* packet, int* out_len);
extern int network_packet_read_int(struct NetworkPacket* packet);
extern long long network_packet_read_longlong(struct NetworkPacket* packet);
*/

extern void network_packet_destroy(struct NetworkPacket* packet);

/* TCP */

extern char* network_tcp_network_address_get(long scope, int port);

extern void network_tcp_socket_create(struct Network* network, const char* network_address, long scope, int network_port);

extern void network_tcp_socket_server_bind(struct Network* network);
extern void network_tcp_socket_server_listen(struct Network* network);
extern void network_tcp_socket_server_accept(struct Network* network, struct Network* client);

extern void network_tcp_socket_client_connect(struct Network* network);

/* UDP Multicast */

extern void network_udp_multicast_socket_server_create(struct Network* network, int network_port);
extern void network_udp_multicast_socket_server_group_join(struct Network* network, const char* network_address);
extern void network_udp_multicast_socket_server_group_drop(struct Network* network, const char* network_address);

extern void network_udp_multicast_socket_client_create(struct Network* network, const char* network_address, int network_port);

#endif /*_NETWORK_H_*/
