#include "socket_interface_cl.h"

#include "paramset.h"
#include "clusterfq_cl.h"
#include "const_defs.h"

#include "../clusterfq/network.h"
#include "../clusterfq/util.h"

#include <sstream>
#include <iostream>

#ifdef _WIN32
#else
#include <cstring>
#include <climits>
#endif

string socket_interface_address = "::1";
int socket_interface_port = 8080;

string socket_interface_query(const char* module, const char* module_action, int paramset_id) {
	struct paramset* ps = paramset__get(paramset_id);
	string result = "";
	if (ps != nullptr || paramset_id == -1) {
		stringstream request;
		request << module << module_action;

		string post_content = "";

		bool questionmark = false;
		string lf = "\r\n";
		if (ps != nullptr) {
			for (int p = 0; p < ps->param_count; p++) {
				if (strstr(ps->params[p].first, ClusterFQ::PARAM_POST) != nullptr) {
					post_content = ps->params[p].second;
				} else {
					if (!questionmark) {
						questionmark = true;
						request << "?";
					} else {
						request << "&";
					}
					request << ps->params[p].first << "=" << ps->params[p].second;
				}
			}
			paramset__remove(paramset_id);
		}
		request << lf;

		if (post_content.length() > 0) {
			request << "Content-Length: " << post_content.length() << lf;
			request << lf;
			request << post_content;
		}
		request << lf;

		struct Network client;
		network_init(&client);
		network_tcp_socket_create(&client, socket_interface_address.c_str(), 0, socket_interface_port);
		network_tcp_socket_client_connect(&client);

		std::cout << request.str();

		client.send(&client, request.str().c_str(), request.str().length());

		char buffer;

		int last_char = 0;
		int line_len = INT_MAX;
		int char_ct = 0;

		char linebuffer[1024];

		vector<string> response;
		while (client.state != NS_ERROR && client.state != NS_NULL) {
			unsigned int out_len = 0;

			client.read(&client, &buffer, 1, nullptr, &out_len);
			if (out_len != 1) {
				util_sleep(16);
				continue;
			}
			last_char = (int)buffer;
			if (char_ct < 1024) {
				linebuffer[char_ct] = buffer;
				char_ct++;
			}
			if (last_char == 10) {
				line_len = char_ct;
				if (line_len == 1) {
					int content_length = 0;
					char* post_content = nullptr;

					for (int i = 1; i < response.size(); i++) {
						vector<string> req_sub = util_split(response[i], " ");
						if (strstr(req_sub[0].c_str(), "Content-Length:") != nullptr) {
							content_length = stoi(req_sub[1].c_str());
							break;
						}
					}
					if (content_length > 0) {
						post_content = (char*)malloc(content_length + 1);
						int tries = 0;
						for (int cc = 0; cc < content_length; cc++) {
							tries = 0;
							do {
								if (tries > 0) util_sleep(16);
								client.read(&client, &buffer, 1, nullptr, &out_len);
								tries++;
							} while (out_len != 1);
							post_content[cc] = buffer;
						}
						post_content[content_length] = '\0';
					}

					//TODO: change to raw out param
					response.push_back(post_content);

					network_destroy(&client);
				} else {
					linebuffer[char_ct] = '\0';
					string tmp(linebuffer);
					response.push_back(tmp);
					memset(linebuffer, 0, char_ct);
				}
				char_ct = 0;
			}
		}
		stringstream response_ss;
		for (int s = 0; s < response.size(); s++) {
			response_ss << response[s];
		}
		result = response_ss.str();
	}
	return result;
}