#include "network.h"

#ifdef _WIN32
#else
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include "util.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

void network_init(struct Network* network) {
	network->socket = -1;
#ifdef _WIN32
	network->ip6addr = (ADDRINFO*)malloc(sizeof(ADDRINFO));
	if (network->ip6addr == NULL) {
		fprintf(stderr, "ERROR: Unable to allocate memory: ip6addr\n");
		network->state = NS_ERROR;
		return;
	}
	network->ip6addr->ai_addr = (struct sockaddr*)malloc(sizeof(struct sockaddr_in6));
	if (network->ip6addr->ai_addr == NULL) {
		fprintf(stderr, "ERROR: Unable to allocate memory: ai_addr\n");
		network->state = NS_ERROR;
		return;
	}
	memset(network->ip6addr->ai_addr, 0, sizeof(struct sockaddr_in6));
#else
	memset(&network->ip6addr, 0, sizeof(network->ip6addr));
#endif
	network->send = NULL;
	network->read = NULL;
	network->state = NS_NULL;
}

void network_destroy(struct Network* network) {
	network->send = NULL;
	network->read = NULL;
#ifdef _WIN32
	closesocket(network->socket);
	free(network->ip6addr->ai_addr);
	free(network->ip6addr);
#else
	close(network->socket);
	memset(&network->ip6addr, 0, sizeof(network->ip6addr));
#endif
	network->socket = -1;
	network->state = NS_NULL;
}

int network_packet_has_space(struct NetworkPacket* packet, size_t size) {
	if (packet->position + size >= packet->max_size) {
		fprintf(stderr, "ERROR: No space in packet\n");
		return 0;
	}
	return 1;
}

int network_packet_alloc_on_demand(struct NetworkPacket* packet, size_t size) {
	if (packet->position + size >= packet->size) {
		packet->data = (char*)realloc(packet->data, packet->size * 2);
		packet->size *= 2;
		if (packet->data == NULL) {
			fprintf(stderr, "ERROR: Unable to realloc space in packet\n");
			return 0;
		}
		return 1;
	}
	return 1;
}

void network_packet_create(struct NetworkPacket* packet, size_t max_size) {
	packet->position = 0;
	if (max_size == 0) {
		packet->data = (char*)malloc(1024);
		packet->size = 1024;
		packet->max_size = 8192;
	}
	else {
		packet->data = (char*)malloc(max_size);
		packet->size = max_size;
		packet->max_size = max_size;
	}
	memset(packet->data, 0, packet->size);
}

void network_packet_create_from_data(struct NetworkPacket* packet, char* data, int size) {
	packet->position = 0;
	packet->size = size;
	packet->data = data;
	packet->max_size = size;
}

void network_packet_append_str(struct NetworkPacket* packet, const char* str, int size) {
	network_packet_append_int(packet, size);
	if (network_packet_has_space(packet, size)) {
		if (network_packet_alloc_on_demand(packet, size)) {
			memcpy((void*)&(packet->data[packet->position]), (void*)str, size);
			packet->position += size;
		}
	}
}

void network_packet_append_int(struct NetworkPacket* packet, int num) {
	if (network_packet_has_space(packet, sizeof(int))) {
		if (network_packet_alloc_on_demand(packet, sizeof(int))) {
			memcpy((void*)&(packet->data[packet->position]), (void*)&num, sizeof(int));
			/*			printf("rereading int: %s\n", network_packet_read_int(packet));*/
			packet->position += sizeof(int);
		}
	}
}

void network_packet_append_longlong(struct NetworkPacket* packet, long long num) {
	if (network_packet_has_space(packet, sizeof(long long))) {
		if (network_packet_alloc_on_demand(packet, sizeof(long long))) {
			memcpy((void*)&(packet->data[packet->position]), (void*)&num, sizeof(long long));
			packet->position += sizeof(long long);
		}
	}
}

char* network_packet_read_str(struct NetworkPacket* packet, int* out_len) {
	char* result;
	(*out_len) = network_packet_read_int(packet);
	result = (char*)malloc((*out_len) + 1);
	if (packet->position + *out_len <= packet->size) {
		memcpy((void*)result, (void*)&(packet->data[packet->position]), *out_len);
		packet->position += *out_len;
	}
	result[*out_len] = '\0';
	return result;
}

int network_packet_read_int(struct NetworkPacket* packet) {
	int result;
	memcpy((void*)&result, (void*)&(packet->data[packet->position]), sizeof(int));
	packet->position += sizeof(int);
	return result;
}

long long network_packet_read_longlong(struct NetworkPacket* packet) {
	long long result;
	memcpy((void*)&result, (void*)&(packet->data[packet->position]), sizeof(long long));
	packet->position += sizeof(long long);
	return result;
}

void network_packet_destroy(struct NetworkPacket* packet) {
	free(packet->data);
	packet->size = 0;
	packet->max_size = 0;
	packet->position = 0;
}

void network_socket_send(struct Network* network, const void* packet, size_t size) {
	int ret;
#ifdef _WIN32
	ret = sendto(network->socket, (const char *)packet, size, 0, (struct sockaddr*)network->ip6addr->ai_addr, network->ip6addr->ai_addrlen);
#else
	ret = sendto(network->socket, packet, size, 0, (struct sockaddr*)&network->ip6addr, sizeof(network->ip6addr));
#endif
	if (ret == -1 || ret != size) {
		fprintf(stderr, "ERROR: Unable to send packet\n");
		network->state = NS_ERROR;
		return;
	}
}

void network_socket_read(struct Network* network, void* packet, size_t size, char *dst_address, unsigned int *out_len) {
	int ret;
#ifdef _WIN32
	if (dst_address == nullptr) {
		ret = recvfrom(network->socket, (char*)packet, size, 0, (struct sockaddr*)&network->ip6addr->ai_addr, (int*)&network->ip6addr->ai_addrlen);
		*out_len = ret;
	} else {
		char ControlBuffer[1024];
		WSABUF WSABuf;
		WSAMSG Msg;
		Msg.name = network->ip6addr->ai_addr;
		Msg.namelen = network->ip6addr->ai_addrlen;
		WSABuf.buf = (char*)packet;
		WSABuf.len = size;
		Msg.lpBuffers = &WSABuf;
		Msg.dwBufferCount = 1;
		Msg.Control.len = sizeof ControlBuffer;
		Msg.Control.buf = ControlBuffer;
		Msg.dwFlags = 0;

		ret = network->WSARecvMsg(network->socket, &Msg, &network->lpcbBytesReturned, NULL, NULL);
		char* pkt = (char*)packet;
		if (ret == 0) {
			if (out_len != nullptr) {
				*out_len = network->lpcbBytesReturned;
			}
			pkt[network->lpcbBytesReturned] = '\0';
			WSACMSGHDR* pCMsgHdr = WSA_CMSG_FIRSTHDR(&Msg);
			while (pCMsgHdr) {
				switch (pCMsgHdr->cmsg_type) {
				case IP_RECVDSTADDR: {
					IN6_PKTINFO* pPktInfo;
					pPktInfo = (IN6_PKTINFO*)WSA_CMSG_DATA(pCMsgHdr);

					in6_addr ia = pPktInfo->ipi6_addr;

					inet_ntop(AF_INET6, (const void*)&pPktInfo->ipi6_addr, dst_address, 45);

					break;
				}
				default: break;
				}
				pCMsgHdr = WSA_CMSG_NXTHDR(&Msg, pCMsgHdr);
			}
		} else {
			pkt[0] = '\0';
			dst_address[0] = '\0';
		}
	}
#else
	if (dst_address == nullptr) {
		ret = read(network->socket, packet, size);
		*out_len = ret;
	} else {
		char cmbuf[0x100];
		struct iovec iov[1];
		struct msghdr mh;
		mh.msg_name = &network->ip6addr;
		mh.msg_namelen = sizeof(network->ip6addr);
		mh.msg_iov = iov;
		mh.msg_iovlen = 1;
		mh.msg_control = cmbuf;
		mh.msg_controllen = sizeof(cmbuf);
		iov[0].iov_base = (char*)packet;
		iov[0].iov_len = size;
		ret = recvmsg(network->socket, &mh, 0);
		char* pkt = (char*)packet;
		if (ret > -1) {
			pkt[ret] = '\0';
			if (out_len != nullptr) *out_len = ret;
		} else {
			pkt[0] = '\0';
			//TODO: check if *out_len should be set tcp & udp
		}

		for (struct cmsghdr* cmsg = CMSG_FIRSTHDR(&mh); cmsg != NULL; cmsg = CMSG_NXTHDR(&mh, cmsg)) {
			if (cmsg->cmsg_type == 50) {
				struct in6_pktinfo* pi = (struct in6_pktinfo*)CMSG_DATA(cmsg);
				inet_ntop(AF_INET6, (const void*)&pi->ipi6_addr, dst_address, 45);
				break;
			}
		}
	}
#endif
	if (ret == -1) {
		fprintf(stderr, "ERROR: Unable to read packet\n");
		network->state = NS_ERROR;
		return;
	}
}

char* network_tcp_network_address_get(long scope, int port) {
	char* network_address;
	const char* tmp;
	if (scope == 2) {
#ifdef _WIN32
		tmp = "fe80::516b:7d16:5d32:8f32";
		network_address = (char*)malloc(strlen(tmp) + 1);
		strncpy(network_address, tmp, strlen(tmp));
		network_address[strlen(tmp)] = '\0';
#else
		tmp = util_issue_command("/sbin/ifconfig | grep \"inet6\" | grep \"fe\" | awk -F' ' '{print $3}' | cut -d'/' -f1");
		if (tmp[4] != ':') {
			tmp = util_issue_command("/sbin/ifconfig | grep \"inet6\" | grep \"fe\" | awk -F' ' '{print $2}' | cut -d'/' -f1");
		}
		if (tmp[4] != ':') {
			fprintf(stdout, "WARNING: No link local ipv6 address found\n");
			tmp = "::";
		}
		network_address = (char*)malloc(strlen(tmp));
		strncpy(network_address, tmp, strlen(tmp) - 1);
		network_address[strlen(tmp) - 1] = '\0';
		/*		network_address = "2a02:908:eb45:5f60:891:9f00:96f8:5aff";*/
#endif
	}
	else {
		network_address = NULL;
	}
	return network_address;
}

void network_tcp_socket_create(struct Network* network, const char* network_address, long scope, int network_port) {
#ifdef _WIN32
	int ret;
	WSADATA wsa_data;
	if ((ret = WSAStartup(MAKEWORD(2, 2), &wsa_data)) != 0) {
		fprintf(stderr, "ERROR: WSAStartup failed with error %d\n", ret);
		WSACleanup();
		network->state = NS_ERROR;
		return;
	}
	ADDRINFO hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET6;
	hints.ai_socktype = SOCK_STREAM;
	char port[6];
	sprintf(&port[0], "%d", network_port);
	ret = getaddrinfo(network_address, port, &hints, &(network->ip6addr));
	if (ret != 0) {
		fprintf(stderr, "ERROR: Cannot resolve address and port, error %d: %s\n", ret, gai_strerror(ret));
		WSACleanup();
		network->state = NS_ERROR;
		return;
	}
#else
	network->ip6addr.sin6_family = AF_INET6;
	network->ip6addr.sin6_port = htons(network_port);
	network->ip6addr.sin6_scope_id = scope;
	if (inet_pton(AF_INET6, network_address, &(network->ip6addr.sin6_addr)) < 1) {
		fprintf(stderr, "ERROR: Cannot resolve address and port\n");
		network->state = NS_ERROR;
		return;
	}
#endif
	network->socket = socket(AF_INET6, SOCK_STREAM, 0);
	if (network->socket < 0) {
		fprintf(stderr, "ERROR: Unable to create socket\n");
		network->state = NS_ERROR;
		return;
	}
	network->state = NS_CREATED;
}

void network_tcp_socket_server_bind(struct Network* network) {
#ifdef _WIN32
	if (bind(network->socket, network->ip6addr->ai_addr, (int)network->ip6addr->ai_addrlen) == SOCKET_ERROR) {
#else
	if (bind(network->socket, (struct sockaddr*)&(network->ip6addr), sizeof(network->ip6addr)) < 0) {
#endif
		fprintf(stderr, "ERROR: Unable to bind socket\n");
		network->state = NS_ERROR;
		return;
	}
	network->state = NS_BOUND;
	}

void network_tcp_socket_server_listen(struct Network* network) {
	if (listen(network->socket, 1) < 0) {
		fprintf(stderr, "ERROR: Unable to listen\n");
		network->state = NS_ERROR;
		return;
	}
	network->state = NS_LISTENING;
}

void network_tcp_socket_server_accept(struct Network* network, struct Network* client) {
	unsigned int len, err;
#ifdef _WIN32
	client->socket = accept(network->socket, client->ip6addr->ai_addr, (int *)&client->ip6addr->ai_addrlen);
#else
	len = sizeof(client->ip6addr);
	client->socket = accept(network->socket, (struct sockaddr*)&(client->ip6addr), &len);
#endif
	if (client->socket < 0) {
#ifdef _WIN32
		err = WSAGetLastError();
#else
		err = errno;
#endif
		fprintf(stderr, "ERROR: Unable to establish connection: %d\n", err);
		client->state = NS_ERROR;
		return;
	}
	client->send = &network_socket_send;
	client->read = &network_socket_read;
	client->state = NS_CONNECTED;
}

void network_tcp_socket_client_connect(struct Network* network) {
#ifdef _WIN32
	if (connect(network->socket, network->ip6addr->ai_addr, (int)network->ip6addr->ai_addrlen) == SOCKET_ERROR) {
#else
	if (connect(network->socket, (struct sockaddr*)&(network->ip6addr), sizeof(network->ip6addr)) < 0) {
#endif
		fprintf(stderr, "ERROR: Unable to connect\n");
		network->state = NS_ERROR;
		return;
	}
	network->send = &network_socket_send;
	network->read = &network_socket_read;
	network->state = NS_CONNECTED;
	}

void network_udp_multicast_socket_server_create(struct Network* network, int network_port) {
	int ret;
	struct ipv6_mreq group;
#ifdef _WIN32
	WSADATA wsa_data;
	if ((ret = WSAStartup(MAKEWORD(2, 2), &wsa_data)) != 0) {
		fprintf(stderr, "ERROR: WSAStartup failed with error %d\n", ret);
		WSACleanup();
		network->state = NS_ERROR;
		return;
	}
	ADDRINFO hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET6;
	hints.ai_socktype = SOCK_DGRAM;
	char port[6];
	sprintf(&port[0], "%d", network_port);
	ret = getaddrinfo("::", port, &hints, &(network->ip6addr));
	if (ret != 0) {
		fprintf(stderr, "ERROR: Cannot resolve address and port, error %d: %s\n", ret, gai_strerror(ret));
		WSACleanup();
		network->state = NS_ERROR;
		return;
	}

#else
	/*
	struct addrinfo hints;
	hints.ai_family = AF_INET6;
	hints.ai_socktype = SOCK_DGRAM;
	struct addrinfo* res, r;
	char port[6];
	sprintf(&port[0], "%d", network_port);
	ret = getaddrinfo("::", port, &hints, &res);
	if (ret != 0) {
		fprintf(stderr, "ERROR: Cannot resolve address and port\n");
		network->state = NS_ERROR;
		return;
	}
	*/

	network->ip6addr.sin6_family = AF_INET6;
	network->ip6addr.sin6_port = htons(network_port);
	network->ip6addr.sin6_addr = in6addr_any;
	/* network->ip6addr.sin6_scope_id = 2; */
#endif
	network->socket = socket(AF_INET6, SOCK_DGRAM, 0);
	if (network->socket < 0) {
		fprintf(stderr, "ERROR: Unable to create socket\n");
		network->state = NS_ERROR;
		return;
	}
	network->state = NS_CREATED;

	int enable = 1;
	if (setsockopt(network->socket, SOL_SOCKET, SO_REUSEADDR, (const char *)&enable, sizeof(int)) < 0)
		fprintf(stderr, "ERROR: Unable to set reuseaddr\n");

	int bufsize = 1024 * 1024 * 5;
	if (setsockopt(network->socket, SOL_SOCKET, SO_RCVBUF, (const char*)&bufsize, sizeof(int)) < 0)
		fprintf(stderr, "ERROR: Unable to set rcvbuf\n");

	if (setsockopt(network->socket, SOL_SOCKET, SO_SNDBUF, (const char*)&bufsize, sizeof(int)) < 0)
		fprintf(stderr, "ERROR: Unable to set rcvbuf\n");

#ifdef _WIN32
	if (bind(network->socket, network->ip6addr->ai_addr, (int)network->ip6addr->ai_addrlen) == SOCKET_ERROR) {
#else
	if (bind(network->socket, (struct sockaddr*)&(network->ip6addr), sizeof(network->ip6addr)) < 0) {
#endif
		fprintf(stderr, "ERROR: Unable to bind socket\n");
		network->state = NS_ERROR;
		return;
	}

	const int on = 1;
#ifdef _WIN32
	ret = setsockopt(network->socket, IPPROTO_IPV6, IPV6_RECVDSTADDR, (const char*)&on, sizeof(on));
#else
	ret = setsockopt(network->socket, IPPROTO_IPV6, IPV6_RECVPKTINFO, (const char*)&on, sizeof(on));
#endif
	if (ret == -1) {
		fprintf(stderr, "ERROR: Unable to recv dstaddr\n");
	}

	network->read = &network_socket_read;
	network->state = NS_BOUND;

#ifdef _WIN32
	GUID WSARecvMsg_GUID = WSAID_WSARECVMSG;

	int nResult;
	nResult = WSAIoctl(network->socket, SIO_GET_EXTENSION_FUNCTION_POINTER,
		&WSARecvMsg_GUID, sizeof WSARecvMsg_GUID,
		&network->WSARecvMsg, sizeof network->WSARecvMsg,
		&network->lpcbBytesReturned, NULL, NULL);
	if (nResult == SOCKET_ERROR) {
		//m_ErrorCode = WSAGetLastError();
		printf("error getting wsarecvmsg function pointer\n");
		network->WSARecvMsg = NULL;
	}
#endif

}

void network_udp_multicast_socket_server_group_join(struct Network* network, const char* network_address) {
	int ret;
	struct ipv6_mreq group;
	group.ipv6mr_interface = 0;
	ret = inet_pton(AF_INET6, network_address, &group.ipv6mr_multiaddr);
	if (ret == 0) {
		fprintf(stderr, "ERROR: Unable to parse network address\n");
		network->state = NS_ERROR;
		return;
	}
	else if (ret == -1) {
		fprintf(stderr, "ERROR: Address family not supported\n");
		network->state = NS_ERROR;
		return;
	}
	ret = setsockopt(network->socket, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, (const char*)&group, sizeof(group));
	if (ret == -1) {
		fprintf(stderr, "ERROR: Unable to set socket options\n");
		network->state = NS_ERROR;
		return;
	}
}

void network_udp_multicast_socket_server_group_drop(struct Network* network, const char* network_address) {
	int ret;
	struct ipv6_mreq group;
	group.ipv6mr_interface = 0;
	ret = inet_pton(AF_INET6, network_address, &group.ipv6mr_multiaddr);
	if (ret == 0) {
		fprintf(stderr, "ERROR: Unable to parse network address\n");
		network->state = NS_ERROR;
		return;
	}
	else if (ret == -1) {
		fprintf(stderr, "ERROR: Address family not supported\n");
		network->state = NS_ERROR;
		return;
	}
	ret = setsockopt(network->socket, IPPROTO_IPV6, IPV6_DROP_MEMBERSHIP, (const char*)&group, sizeof(group));
	if (ret == -1) {
		fprintf(stderr, "ERROR: Unable to set socket options\n");
		network->state = NS_ERROR;
		return;
	}
}

void network_udp_multicast_socket_client_create(struct Network* network, const char* network_address, int network_port) {
	int ret;
#ifdef _WIN32
	WSADATA wsa_data;
	if ((ret = WSAStartup(MAKEWORD(2, 2), &wsa_data)) != 0) {
		fprintf(stderr, "ERROR: WSAStartup failed with error %d\n", ret);
		WSACleanup();
		network->state = NS_ERROR;
		return;
	}
	ADDRINFO hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET6;
	hints.ai_socktype = SOCK_DGRAM;
	char port[6];
	sprintf(&port[0], "%d", network_port);
	ret = getaddrinfo(network_address, port, &hints, &(network->ip6addr));
	if (ret != 0) {
		fprintf(stderr, "ERROR: Cannot resolve address and port, error %d: %s\n", ret, gai_strerror(ret));
		WSACleanup();
		network->state = NS_ERROR;
		return;
	}
#else
	network->ip6addr.sin6_family = AF_INET6;
	network->ip6addr.sin6_port = htons(network_port);
	/*	network->ip6addr.sin6_scope_id = 2; */
	ret = inet_pton(AF_INET6, network_address, &(network->ip6addr.sin6_addr));
	if (ret == 0) {
		fprintf(stderr, "ERROR: Unable to parse network address\n");
		network->state = NS_ERROR;
		return;
	}
	else if (ret == -1) {
		fprintf(stderr, "ERROR: Address family not supported\n");
		network->state = NS_ERROR;
		return;
	}
#endif
	network->socket = socket(AF_INET6, SOCK_DGRAM, 0);
	if (network->socket < 0) {
		fprintf(stderr, "ERROR: Unable to create socket\n");
		network->state = NS_ERROR;
		return;
	}

	int bufsize = 1024 * 1024 * 5;
	if (setsockopt(network->socket, SOL_SOCKET, SO_RCVBUF, (const char*)&bufsize, sizeof(int)) < 0)
		fprintf(stderr, "ERROR: Unable to set rcvbuf\n");

	if (setsockopt(network->socket, SOL_SOCKET, SO_SNDBUF, (const char*)&bufsize, sizeof(int)) < 0)
		fprintf(stderr, "ERROR: Unable to set rcvbuf\n");

	network->send = &network_socket_send;
	network->state = NS_CREATED;
}
