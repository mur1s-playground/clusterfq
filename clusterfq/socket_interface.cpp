#include "socket_interface.h"

#include "thread.h"
#include <sstream>
#include <vector>
#include "util.h"
#include <time.h>
#include "identity.h"
#include <iostream>

#ifdef _WIN32

#else
#include <cstring>
#endif


using namespace std;

struct socket_interface socket_interface_static;
bool					socket_interface_running = true;
struct ThreadPool		socket_interface_thread_pool;

void socket_interface_static_init(int port) {
	thread_pool_init(&socket_interface_thread_pool, 25);

	network_init(&socket_interface_static.tcp_server);
	network_tcp_socket_create(&socket_interface_static.tcp_server, "::", 0, port);
	network_tcp_socket_server_bind(&socket_interface_static.tcp_server);
	network_tcp_socket_server_listen(&socket_interface_static.tcp_server);
}

const char* HTTP_RESPONSE_200 = "HTTP/1.1 200 OK\n";
const char* HTTP_RESPONSE_404 = "HTTP/1.1 404 Not found\n";
const char* HTTP_RESPONSE_501 = "HTTP/1.1 501 Not implemented\n";
const char* HTTP_RESPONSE_SERVER            = "Server: clusterfq\n";
const char* HTTP_RESPONSE_CONTENT_TYPE_JSON = "Content-Type: application/json\n";
const char* HTTP_RESPONSE_CONNECTION_CLOSED = "Connection: closed\n";
const char* HTTP_RESPONSE_CORS              = "Access-Control-Allow-Origin: *\n";


string http_response_date_now() {
    stringstream datenow;
    datenow << "Date: ";

    time_t now = time(nullptr);

    tm* gmtm = gmtime(&now);

    string gmt(asctime(gmtm));
    gmt = util_trim(gmt, "\r\n\t ");
    datenow << gmt << " UTC\n";

    string result = datenow.str();
    return result;
}

string http_request_get_param(vector<string>* params, string param) {
    string result = "";
    for (int p = 0; p < params->size(); p++) {
        vector<string> splt = util_split((*params)[p].c_str(), "=");
        const char* sparam = splt[0].c_str();
        if (strstr(sparam, param.c_str()) == sparam && splt.size() > 1) {
            return splt[1];
        }
    }
    return result;
}

void socket_interface_process_client(void* param) {
    std::cout << "processing request started\n";

    struct Network* client = (struct Network*)param;
    char buffer;

    int last_char = 0;
    int line_len = INT_MAX;
    int char_ct = 0;

    char linebuffer[1024];

    vector<string> request = vector<string>();

    while (client->state != NS_ERROR) {
        enum socket_interface_request_type sirt = SIRT_GET;

        unsigned int out_len = 0;
        network_socket_read(client, &buffer, 1, nullptr, &out_len);
        if (out_len != 1) {
            util_sleep(16);
            continue;
        }
        std::cout << buffer;
        last_char = (int)buffer;
        linebuffer[char_ct] = buffer;
        char_ct++;
        if (last_char == 13) {
            line_len = char_ct;

            if (line_len == 2) {
                string request_url("/");
                vector<string> request_path = vector<string>();
                vector<string> params = vector<string>();

                stringstream post_content;

                stringstream response;

                if (request.size() > 0) {
                    vector<string> req_0_splt = util_split(request[0], " ");

                    if (req_0_splt.size() > 1) {
                        request_url = req_0_splt[1];

                        vector<string> url_param_split = util_split(request_url, "?");
                        request_path = util_split(url_param_split[0].c_str(), "/");
                        request_path.erase(request_path.begin());

                        if (url_param_split.size() > 1) {
                            params = util_split(url_param_split[1], "&");
                        }
                    }

                    const char *req_0 = request[0].c_str();
                    if (strstr(req_0, "GET") == req_0) {
                        sirt = SIRT_GET;
                    } else if (strstr(req_0, "POST") == req_0) {
                        sirt = SIRT_POST;
                        
                        int content_length = 0;
                        for (int i = 1; i < request.size(); i++) {
                            vector<string> req_sub = util_split(request[i], " ");
                            if (strstr(req_sub[0].c_str(), "Content-Length:") != nullptr) {
                                content_length = stoi(req_sub[1].c_str());
                                break;
                            }
                        }

                        if (content_length > 0) {
                            network_socket_read(client, &buffer, 1, nullptr, &out_len);
                            for (int cc = 0; cc < content_length; cc++) {
                                network_socket_read(client, &buffer, 1, nullptr, &out_len);
                                post_content << buffer;
                            }
                        }
                    } else if (strstr(req_0, "OPTIONS") == req_0) {
                        sirt = SIRT_OPTIONS;
                    }
                }

                string content = "{ }\n";

                char** status_code = (char **)&HTTP_RESPONSE_501;
                if (request_path.size() > 0) {
                    const char* rp_0 = request_path[0].c_str();
                    string pc_s = post_content.str();
                    if (strstr(rp_0, "packetset") == rp_0) {
                        content = packetset_interface(sirt, &request_path, &params, pc_s, status_code);
                    } else if (strstr(rp_0, "identity") == rp_0) {
                        content = identity_interface(sirt, &request_path, &params, pc_s, status_code);
                    } else if (strstr(rp_0, "contact") == rp_0) {
                        content = contact_interface(sirt, &request_path, &params, pc_s, status_code);
                    } else if (strstr(rp_0, "message") == rp_0) {
                        content = message_interface(sirt, &request_path, &params, pc_s, status_code);
                    }
                }
                response << *status_code;
                response << http_response_date_now();
                response << HTTP_RESPONSE_CORS;
                if (sirt == SIRT_OPTIONS) {
                    response << content;
                } else {
                    response << HTTP_RESPONSE_CONTENT_TYPE_JSON;
                    response << "Content-Length: " << content.length() << "\n";
                }
                response << HTTP_RESPONSE_CONNECTION_CLOSED;
                response << "\n";
                if (sirt != SIRT_OPTIONS) {
                    response << content;
                }

                std::cout << response.str();

                network_socket_send(client, response.str().c_str(), response.str().length());

                network_socket_read(client, &buffer, 1, nullptr, &out_len);
                request.clear();
                response.clear();
                content = "";
            } else {
                linebuffer[char_ct] = '\0';
                string tmp(linebuffer);
                request.push_back(tmp);
                memset(linebuffer, 0, char_ct);
            }
            char_ct = 0;
        }
    }
    network_destroy(client);
    free(client);
    std::cout << "processing request done\n";
}

void socket_interface_listen_loop() {
	while (socket_interface_running) {
		struct Network* client = new struct Network();
		network_init(client);
		network_tcp_socket_server_accept(&socket_interface_static.tcp_server, client);
		thread_create(&socket_interface_thread_pool, (void*)&socket_interface_process_client, client);
	}
}