#include "network.h"

#ifdef _WIN32
#else
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "util.h"
#include <sstream>

#include "address_factory.h"
#include <iostream>

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

bool network_packet_read_str(struct NetworkPacket* packet, char **out, int* out_len) {
	if (network_packet_read_int(packet, out_len)) {
		if (*out_len >= 0 && packet->position + *out_len <= packet->size) {
			char* result = (char*)malloc(*out_len + 1);
			*out = result;
			memcpy(result, (void*)&(packet->data[packet->position]), *out_len);
			packet->position += *out_len;
			result[*out_len] = '\0';
			return true;
		}
	}
	return false;
}

/*
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
*/

bool network_packet_read_int(struct NetworkPacket* packet, int* out) {
	if (packet->position + sizeof(int) <= packet->size) {
		memcpy(out, (void*)&(packet->data[packet->position]), sizeof(int));
		packet->position += sizeof(int);
		return true;
	}
	return false;
}

/*
int network_packet_read_int(struct NetworkPacket* packet) {
	int result;
	memcpy((void*)&result, (void*)&(packet->data[packet->position]), sizeof(int));
	packet->position += sizeof(int);
	return result;
}
*/

bool network_packet_read_longlong(struct NetworkPacket* packet, long long* out) {
	if (packet->position + sizeof(long long) <= packet->size) {
		memcpy(out, (void*)&(packet->data[packet->position]), sizeof(long long));
		packet->position += sizeof(long long);
		return true;
	}
	return false;
}

/*
long long network_packet_read_longlong(struct NetworkPacket* packet) {
	long long result;
	memcpy((void*)&result, (void*)&(packet->data[packet->position]), sizeof(long long));
	packet->position += sizeof(long long);
	return result;
}
*/

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

bool network_address_is_valid(string address) {
	if (address.length() < 7) return false;

	const char symbols[17] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f', ':' };

	if (address.at(0) == ':') return false;
	if (address.at(address.length() - 1) == ':') return false;

	int nc = 0;
	int segments = 1;
	bool double_found = false;

	for (int i = 4; i < address.length(); i++) {
		bool found = false;
		for (int s = 0; s < 17; s++) {
			if (address.at(i) == symbols[s]) {
				if (address.at(i) == ':') {
					if (i + 1 < address.length() && address.at(i + 1) == ':') {
						if (double_found) {
							return false;
						}
						double_found == true;
						i++;
					}
					segments++;
					if (segments > 8) {
						return false;
					}
					nc = 0;
				} else {
					nc++;
					if (nc > 4) {
						return false;
					}
				}
				found = true;
				break;
			}
		}
		if (!found) {
			return false;
		}
	}
	if (segments < 2) {
		return false;
	}
	return true;
}

enum NetworkAddressOrigin network_address_parse_origin(const char* val) {
	if (strstr(val, "WellKnown") == val) {
		return NAPO_WELLKNOWN;
	} else if (strstr(val, "DHCP") == val) {
		return NAPO_DHCP;
	} else if (strstr(val, "Manual") == val) {
		return NAPO_MANUAL;
	} else if (strstr(val, "RouterAdvertisement") == val) {
		return NAPO_ROUTERADVERTISEMENT;
	} else if (strstr(val, "Link") == val) {
		return NAPO_LINK;
	} else if (strstr(val, "Random") == val) {
		return NAPO_RANDOM;
	}
	return NAPO_OTHER;
}

enum NetworkAddressState network_address_parse_state(const char* val) {
	if (strstr(val, "Preferred") == val) {
		return NAS_PREFERRED;
	} else if (strstr(val, "Tentative") == val) {
		return NAS_TENTATIVE;
	} else if (strstr(val, "Duplicate") == val) {
		return NAS_DUPLICATE;
	} else if (strstr(val, "Deprecated") == val) {
		return NAS_DEPRECATED;
	}
	return NAS_INVALID;
}

time_t network_address_parse_lifetime(vector<string> &splt) {
	return time(nullptr) + (long long)stoi(splt[3]) + 60l * ((long long)stoi(splt[2]) + 60l * (long long)stoi(splt[1]));
}

bool network_address_in_range(string address, string from, string to) {
	for (int i = 0; i < address.length(); i++) {
		if (address.at(i) < from.at(i) || address.at(i) > to.at(i)) {
			return false;
		}
		if (address.at(i) < to.at(i)) {
			return true;
		}
	}
	return true;
}

void network_address_dump(struct NetworkAddress* na) {
	vector<string> origins{ "NAPO_OTHER", "NAPO_MANUAL", "NAPO_WELLKNOWN", "NAPO_DHCP", "NAPO_LINK", "NAPO_RANDOM", "NAPO_ROUTERADVERTISEMENT" };
	vector<string> states{ "NAS_INVALID", "NAS_TENTATIVE","NAS_DUPLICATE", "NAS_DEPRECATED", "NAS_PREFERRED" };
	vector<string> scopes{ "NASC_LOOPBACK", "NASC_LINK_LOCAL_UNICAST", "NASC_UNIQUE_LOCAL_UNICAST", "NASC_MULTICAST", "NASC_GLOBAL_UNICAST" };

	std::cout << "addr:\t\t\t" << na->address << std::endl;
	std::cout << "scope:\t\t\t" << scopes[na->scope] << std::endl;
	std::cout << "if_id:\t\t\t" << na->interface_id << std::endl;
	std::cout << "if_alias:\t\t" << na->interface_alias << std::endl;
	std::cout << "is_unicast:\t\t" << na->is_unicast << std::endl;
	std::cout << "prefix_len:\t\t" << na->prefix_length << std::endl;
	std::cout << "prefix_orig:\t\t" << origins[na->prefix_origin] << std::endl;
	std::cout << "suffix_orig:\t\t" << origins[na->suffix_origin] << std::endl;
	std::cout << "state:\t\t\t" << states[na->state] << std::endl;

	string valid_t = "infinite";
	if (na->valid_lifetime > 0) {
		tm* valid_tm = gmtime(&na->valid_lifetime);
		valid_t = asctime(valid_tm);
		valid_t = util_trim(valid_t, "\r\n\t ") + " UTC";
	}
	std::cout << "valid_lifetime:\t\t" << valid_t << std::endl;

	string pref_t = "infinite";
	if (na->preferred_lifetime > 0) {
		tm* pref_tm = gmtime(&na->preferred_lifetime);
		pref_t = asctime(pref_tm);
		pref_t = util_trim(pref_t, "\r\n\t ") + " UTC";
	}
	std::cout << "preferred_lifetime:\t" << pref_t << std::endl;;

	std::cout << "skipassource:\t\t" << na->skipassource << std::endl;
	std::cout << "connected:\t\t" << na->connected << std::endl << std::endl;
}

enum NetworkAddressScope network_address_get_nonlocal_scope(string address) {
	string from_link_local = "fe80:0000:0000:0000:0000:0000:0000:0000";
	string to_link_local = "febf:0000:0000:0000:0000:0000:0000:0000";
	if (network_address_in_range(address, from_link_local, to_link_local)) {
		return NASC_LINK_LOCAL_UNICAST;
	}
	string from_unique_link_local = "fc00:0000:0000:0000:0000:0000:0000:0000";
	string to_unique_link_local = "fdff:0000:0000:0000:0000:0000:0000:0000";
	if (network_address_in_range(address, from_unique_link_local, to_unique_link_local)) {
		return NASC_UNIQUE_LOCAL_UNICAST;
	}
	string from_multicast = "ff00:0000:0000:0000:0000:0000:0000:0000";
	string to_multicast = "ffff:0000:0000:0000:0000:0000:0000:0000";
	if (network_address_in_range(address, from_multicast, to_multicast)) {
		return NASC_MULTICAST;
	}
	return NASC_LOOPBACK;
}

vector<struct NetworkAddress> network_address_get() {
	vector<struct NetworkAddress> result = vector<struct NetworkAddress>();

#ifdef _WIN32
		int interface_id = -1;

		char *tmp = util_issue_command("PowerShell.exe -Command \"Get-NetIPAddress -AddressFamily IPv6\"");
		vector<string> lines = util_split(string(tmp), "\r\n");

		struct NetworkAddress na;
		bool next_interface = true;

		for (int i = 0; i < lines.size(); i++) {
			if (next_interface) {
				na.address = "";
				na.scope = NASC_GLOBAL_UNICAST;
				na.interface_alias = "";
				na.interface_id = -1;
				na.is_unicast = false;
				na.preferred_lifetime = 0;
				na.valid_lifetime = 0;
				na.prefix_length = 0;
				na.prefix_origin = NAPO_OTHER;
				na.skipassource = false;
				na.state = NAS_INVALID;
				na.suffix_origin = NAPO_OTHER;
				na.connected = false;
				next_interface = false;
			}

			vector<string> splt = util_split(lines[i], ":");

			if (splt.size() > 1) {
				string pname = splt[0];
				pname = util_trim(pname, "\r\n\t ");
				const char* pname_c = pname.c_str();

				string value = splt[1];
				value = util_trim(value, "\r\n\t ");
				const char* val = value.c_str();

				if (strstr(pname_c, "IPAddress") == pname_c) {
					stringstream address;
					splt[1] = util_trim(splt[1], "\r\n\t ");

					if (splt[1].length() == 0) {
						address << "0000:0000:0000:0000:0000:0000:0000:0001";
						na.scope = NASC_LOOPBACK;
					} else {
						for (int j = 1; j < splt.size(); j++) {
							string tmp_ = splt[j];
							tmp_ = util_trim(tmp_, "\r\n\t ");

							const char* tmp_c = tmp_.c_str();
							if (strstr(tmp_c, "%") != nullptr) {
								tmp_ = tmp_.substr(0, strstr(tmp_c, "%") - tmp_c);
							}

							address << tmp_;
							if (j + 1 < splt.size()) {
								address << ":";
							}
						}
					}
					char* addr = (char *)malloc(45 + 1);
					memcpy(addr, address.str().data(), address.str().length());
					addr[address.str().length()] = '\0';
					util_ipv6_address_to_normalform(addr);
					na.address = string(addr);
					free(addr);
					if (na.scope != NASC_LOOPBACK) {
						na.scope = network_address_get_nonlocal_scope(na.address);
					}
					
					const char* addr_c = na.address.c_str();
				} else if (strstr(pname_c, "InterfaceIndex") == pname_c) {
					na.interface_id = stoi(value);

					stringstream con_com;
					con_com << "PowerShell.exe -Command \"Get-WmiObject Win32_NetworkAdapter -filter 'InterfaceIndex = " << na.interface_id << "' | select netconnectionstatus\"";

					char* tmp_con = util_issue_command(con_com.str().c_str());

					vector<string> lines_con = util_split(tmp_con, "\r\n");
					if (lines_con.size() == 7) {
						string tmp_v = lines_con[3];
						tmp_v = util_trim(lines_con[3], "\r\n\t ");
						if (tmp_v.length() > 0) {
							if (stoi(tmp_v) == 2) {
								na.connected = true;
							}
						}
					}
					free(tmp_con);

				} else if (strstr(pname_c, "InterfaceAlias") == pname_c) {
					na.interface_alias = value;
				} else if (strstr(pname_c, "Type") == pname_c) {
					if (strstr(val, "Unicast") == val) {
						na.is_unicast = true;
					} else {
						na.is_unicast = false;
					}
				} else if (strstr(pname_c, "PrefixLength") == pname_c) {
					na.prefix_length = stoi(value);
				} else if (strstr(pname_c, "PrefixOrigin") == pname_c) {
					na.prefix_origin = network_address_parse_origin(val);
				} else if (strstr(pname_c, "SuffixOrigin") == pname_c) {
					na.suffix_origin = network_address_parse_origin(val);
				} else if (strstr(pname_c, "AddressState") == pname_c) {
					na.state = network_address_parse_state(val);
				} else if (strstr(pname_c, "ValidLifetime") == pname_c) {
					if (strstr(val, "Infinite") == val) {
						na.valid_lifetime = 0;
					} else {
						na.valid_lifetime = network_address_parse_lifetime(splt);
					}
				} else if (strstr(pname_c, "PreferredLifetime") == pname_c) {
					if (strstr(val, "Infinite") == val) {
						na.preferred_lifetime = 0;
					} else {
						na.preferred_lifetime = network_address_parse_lifetime(splt);
					}
				} else if (strstr(pname_c, "SkipAsSource") == pname_c) {
					if (strstr(val, "False") == val) {
						na.skipassource = false;
					} else {
						na.skipassource = true;
					}
					next_interface = true;
					result.push_back(na);
				}
			}
		}
		free(tmp);
#else
		char* tmp = util_issue_command("/sbin/ip -6 addr show");
		
		vector<string> lines = util_split(string(tmp), "\n");

		struct NetworkAddress na;
		int na_c = 0;

		for (int l = 0; l < lines.size(); l++) {
			vector<string> splt = util_split(lines[l], " ");
			if (splt.size() > 7) {
				string tmp = util_trim(splt[0], "\r\n\t ");
				string isinet6 = util_trim(splt[4], "\r\n\t ");
				string isvalid_lft = util_trim(splt[7], "\r\n\t ");
					if (tmp.length() > 0 && tmp.at(tmp.length() - 1) == ':') { //interface line;
						na.address = "";
						na.scope = NASC_GLOBAL_UNICAST;
						na.is_unicast = false;
						na.preferred_lifetime = 0;
						na.valid_lifetime = 0;
						na.prefix_length = 0;
						na.state = NAS_INVALID;
						na.prefix_origin = NAPO_OTHER;
						na.skipassource = false;
						na.suffix_origin = NAPO_OTHER;
						na.connected = false;

						na.interface_id = stoi(tmp.substr(0, tmp.length() - 1).c_str());

						string ia = util_trim(splt[1], "\r\n\t ");

						na.interface_alias = ia.substr(0, ia.length() - 1);

						string state = util_trim(splt[5], "\r\n\t ");

						if (strstr(state.c_str(), "state") != nullptr) {
							string st = util_trim(splt[6], "\r\n\t ");
							if (strstr(st.c_str(), "UNKNOWN") != nullptr) {
								na.connected = false;
							} else if (strstr(st.c_str(), "UP") != nullptr) {
								na.connected = true;
							}
						}
					} else if (strstr(isinet6.c_str(), "inet6") != nullptr) { //inet6 line
						string addrs = util_trim(splt[5], "\r\n\t ");
						vector<string> addr_s = util_split(addrs, "/");

						if (addr_s[0].length() == 3) {
							na.address = "0000:0000:0000:0000:0000:0000:0000:0001";
							na.scope = NASC_LOOPBACK;
						} else {
							char* tmp_addr = (char *)malloc(100);
							memcpy(tmp_addr, addr_s[0].data(), addr_s[0].length());
							tmp_addr[addr_s[0].length()] = '\0';
							util_ipv6_address_to_normalform(tmp_addr);
							na.address = string(tmp_addr);
							free(tmp_addr);
							na.scope = network_address_get_nonlocal_scope(na.address);
						}

						na.prefix_length = stoi(addr_s[1]);
					} else if (strstr(isvalid_lft.c_str(), "valid_lft") != nullptr) { //lifetime line
						string t1 = util_trim(splt[8], "\r\n\t ");
						if (strstr(t1.c_str(), "forever") != nullptr) {
							na.valid_lifetime = 0;
						} else {
							vector<string> t1_s = util_split(t1, "sec");
							na.valid_lifetime = stoi(t1_s[0].c_str());
						}

						string t2 = util_trim(splt[10], "\r\n\t ");
						if (strstr(t2.c_str(), "forever") != nullptr) {
							na.preferred_lifetime = 0;
						} else {
							vector<string> t2_s = util_split(t2, "sec");
							na.preferred_lifetime = stoi(t2_s[0].c_str());
						}
						if (na.scope == NASC_GLOBAL_UNICAST || na.scope == NASC_LINK_LOCAL_UNICAST || na.scope == NASC_UNIQUE_LOCAL_UNICAST) {
							na.is_unicast = true;
						}
						result.push_back(na);
					}
			}
		}
#endif
	return result;
}

string network_address_get_random_in_subnet(struct NetworkAddress* na, const unsigned char* random_data) {
	string prefix = "";
	int pos = 0;
	int nc = 0;
	int segments = 0;
	for (int i = 0; i < na->prefix_length / 4; i++) {
		prefix += na->address.at(pos);
		pos++;
		nc++;
		if ((i+1) % 4 == 0) {
			prefix += na->address.at(pos);
			pos++;
			nc = 0;
			segments++;
		}
	}
	int m = na->prefix_length % 4;

	string subnet_from = "";
	string subnet_to = "";

	const char symbols[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };

	if (m == 0) {
		subnet_from = prefix;
		subnet_to = prefix;		
	} else if (m > 0) {
		int p_val = 0;
		for (int s = 0; s < 16; s++) {
			if (na->address.at(pos) == symbols[s]) {
				p_val = s;
			}
		}

		int bits[4] = {0, 0, 0, 0};
		for (int m_i = 0; m_i < 4; m_i++) {
			bits[3 - m_i] = p_val % 2;
			p_val /= 2;
		}

		int bits_r[4] = { 0, 0, 0, 0 };
		int bits_h[4] = { 0, 0, 0, 0 };
		bool maxxed = true;
		int lower_val = 0;
		int mult = 8;
		for (int m_i = 0; m_i < m; m_i++) {
			if (bits[m_i] == 1) {
				bits_r[m_i] = 1;
				bits_h[m_i] = 1;
				lower_val += mult * bits_r[m_i];
			} else {
				maxxed = false;
			}
			mult /= 2;
		}
		subnet_from = prefix;
		subnet_from += symbols[lower_val];

		if (maxxed) {
			subnet_to = prefix;
			subnet_to += 'f';
		} else {
			for (int m_i = m - 1; m_i >= 0; m_i--) {
				if (bits_r[m_i] == 0) {
					bits_h[m_i] = 1;
					break;
				} else {
					bits_h[m_i] = 0;
				}
			}

			int higher_val = 0;
			int mult = 8;
			for (int m_i = 0; m_i < m; m_i++) {
				higher_val += mult * bits_h[m_i];
				mult /= 2;
			}
			higher_val--;
			subnet_to = prefix;
			subnet_to += symbols[higher_val];
		}
		nc++;
		if (nc == 3) {
			subnet_from += ':';
			subnet_to += ':';
			nc = 0;
			segments++;
		}
	}

	for (int segments_i = segments; segments_i < 8; segments_i++) {
		for (int nc_i = nc; nc_i < 4; nc_i++) {
			subnet_from += '0';
			subnet_to += 'f';
		}
		if (segments_i + 1 < 8) {
			subnet_from += ":";
			subnet_to += ":";
		}
		nc = 0;
	}

	std::cout << "address: " << na->address << "/" << na->prefix_length <<std::endl;
	std::cout << "subnet_from: " << subnet_from << std::endl;
	std::cout << "subnet_to: " << subnet_to << std::endl;

	const unsigned char* addr_rand = random_data;
	int idx = 0;

	string random_addr = "";
	
	for (int i = 0; i < 39; i++) {
		if (subnet_from.at(i) == subnet_to.at(i)) {
			random_addr += subnet_from.at(i);
		} else {
			int range_to = 0;
			int range_from = 0;
			for (int s = 0; s < 16; s++) {
				if (symbols[s] == subnet_from.at(i)) {
					range_from = s;
				}
				if (symbols[s] == subnet_to.at(i)) {
					range_to = s;
					break;
				}
			}
			int range = range_to - range_from;
			int val = addr_rand[i] % range;
			random_addr += symbols[range_from + val];
		}
	}
	return random_addr;
}

void network_address_add(int interface_id, string interface_alias, string address, string prefix_length) {
	stringstream cmd;
#ifdef _WIN32
	cmd << "clusterfq_elevated.exe add_address " << interface_id << " " << address << " " << prefix_length;
#else
	cmd << "./clusterfq_elevated add_address " << interface_alias << " " << address << " " << prefix_length;
#endif
	const char* tmp = util_issue_command(cmd.str().c_str());
	std::cout << tmp << std::endl;
}

void network_address_delete(int interface_id, string interface_alias, string address, string prefix_length) {
	stringstream cmd;
#ifdef _WIN32
	cmd << "clusterfq_elevated.exe delete_address " << interface_id << " " << address;
#else
	cmd << "./clusterfq_elevated delete_address " << interface_alias << " " << address << " " << prefix_length;
#endif
	const char* tmp = util_issue_command(cmd.str().c_str());
	std::cout << tmp << std::endl;
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
