#include "lossy_pipe.h"

#include "util.h"

#include <sstream>
#include <iostream>

#ifdef _WIN32
#else
#include <cstring>
#endif


struct ThreadPool lossy_pipes_thread_pool;
vector<struct lossy_pipe*>	lossy_pipes = vector<struct lossy_pipe*>();

void lossy_pipe_static_init() {
    thread_pool_init(&lossy_pipes_thread_pool, 20);
}

bool lossy_pipe_init(struct lossy_pipe* lp, string descriptor, int mtu, unsigned char** out_address, int* out_port, struct Key* k) {
    std::cout << "lossy_pipe_init" << std::endl;

    lp->descriptor = descriptor;
    lp->mtu = mtu;

    vector<struct NetworkAddress> na = network_address_get();

    int na_i = -1;

    for (int n = 0; n < na.size(); n++) {
        if (na[n].state == NAS_DEPRECATED) continue;
        if (na[n].scope == NASC_GLOBAL_UNICAST) {
            na_i = n;
            break;
        }
        if (na[n].scope == NASC_UNIQUE_LOCAL_UNICAST) {
            na_i = n;
        }
        if (na_i == -1 && na[n].scope == NASC_LINK_LOCAL_UNICAST) {
            na_i = n;
        }
        network_address_dump(&na[n]);
    }

    int port_base = 1338;
    int port_offset = 0;

    while (port_offset < 60) {
        network_init(&lp->n);
        network_udp_unicast_socket_server_create(&lp->n, "::", port_base + port_offset);
        if (lp->n.state == NS_ERROR) {
            network_destroy(&lp->n);
            port_offset++;
        }
        else {
            break;
        }
    }
    if (lp->n.state != NS_BOUND) {
        return false;
    }

    unsigned char* out_addr = (unsigned char*)malloc(na[na_i].address.length() + 1);
    *out_address = out_addr;
    memcpy(out_addr, na[na_i].address.data(), na[na_i].address.length());
    out_addr[na[na_i].address.length()] = '\0';

    *out_port = port_base + port_offset;

    std::cout << "lp server created: " << na[na_i].address << " : " << (port_base + port_offset) << std::endl;

    stringstream smb_name;
    smb_name << descriptor << "_" << (port_base + port_offset);

    std::cout << "smb initing" << std::endl;

    if (shared_memory_buffer_init(&lp->smb, smb_name.str(), mtu, 200, 0)) {

        std::cout << "smb inited" << std::endl;

        struct lossy_pipe_loop_params* lplp = new lossy_pipe_loop_params();
        lplp->lp = lp;
        lplp->thread_id = thread_create(&lossy_pipes_thread_pool, (void*)&lossy_pipe_loop, (void*)lplp);

        lp->k = crypto_key_copy(k);
        return true;
    }
    else {
        std::cout << "smb error" << std::endl;
        network_destroy(&lp->n);
        free(*out_address);
        *out_address = nullptr;
    }

    return false;
}

bool lossy_pipe_client_init(struct lossy_pipe* lp, string descriptor, int mtu, unsigned char* address, int port, struct Key* k) {
    lp->descriptor = descriptor;
    lp->mtu = mtu;

    network_init(&lp->n);
    network_udp_multicast_socket_client_create(&lp->n, (char*)address, port);

    std::cout << "lp client created: " << address << " : " << port << std::endl;

    lp->k = crypto_key_copy(k);

    stringstream smb_name;
    smb_name << descriptor << "_" << port << "_in";

    shared_memory_buffer_init(&lp->smb, smb_name.str(), mtu, 200, 0);

    struct lossy_pipe_loop_params* lplp = new lossy_pipe_loop_params();
    lplp->lp = lp;
    lplp->thread_id = thread_create(&lossy_pipes_thread_pool, (void*)&lossy_pipe_send_loop, (void*)lplp);

    return true;
}

void lossy_pipe_loop(void* param) {
    std::cout << "lp loop started" << std::endl;

    struct lossy_pipe_loop_params* lplp = (struct lossy_pipe_loop_params*)param;
    struct lossy_pipe* lp = lplp->lp;
    int current_slot = 0;
    unsigned char* pkt_buffer = (unsigned char*)malloc(lp->mtu);
    unsigned char* pkt_buffer_dec = (unsigned char*)malloc(lp->mtu);
    pkt_buffer[lp->mtu - 1] = '\0';

    time_t t1 = time(nullptr);
    int pkt_ct = 0;

    while (lp->n.state != NS_ERROR) {
        unsigned int out_len = 0;
        lp->n.read(&lp->n, pkt_buffer, lp->mtu, nullptr, &out_len);
        if (out_len > 0) {
            int dec_out_len = 0;
            pkt_buffer_dec = crypto_key_sym_decrypt(lp->k, pkt_buffer, out_len, &dec_out_len, pkt_buffer_dec);
            if (dec_out_len > 0) {
                if (dec_out_len < lp->mtu) {
                    int c_s = current_slot;
                    do {
                        c_s = shared_memory_buffer_write_slot(&lp->smb, current_slot, pkt_buffer_dec, dec_out_len);
                        if (c_s == -1) util_sleep(1);
                    } while (c_s == -1);
                    current_slot = (c_s + 1) % lp->smb.slots;
                    pkt_ct++;
                }
            }
        }
        time_t t2 = time(nullptr);
        if (t1 != t2) {
            t1 = t2;
            std::cout << pkt_ct << "pkts/s" << std::endl;
            pkt_ct = 0;
        }
    }
    free(pkt_buffer);
    free(pkt_buffer_dec);
    thread_terminated(&lossy_pipes_thread_pool, lplp->thread_id);
    free(lplp);
    std::cout << "lp loop end" << std::endl;
}

void lossy_pipe_send_loop(void* param) {
    struct lossy_pipe_loop_params* lplp = (struct lossy_pipe_loop_params*)param;
    struct lossy_pipe* lp = lplp->lp;
    int current_slot = 0;
    unsigned char* pkt_buffer = (unsigned char*)malloc(lp->mtu);
    unsigned char* pkt_buffer_enc = (unsigned char*)malloc(lp->mtu);

    int nothing_ct = 0;
    while (lp->n.state != NS_ERROR) {
        if (shared_memory_buffer_read_slot(&lp->smb, current_slot, pkt_buffer)) { //BROKEN
            nothing_ct = 0;
            int enc_len = 0;
            int* pkt_len = (int*)&lp->smb.buffer[current_slot * (lp->smb.size + SHAREDMEMORYBUFFER_SLOT_META_SIZE) + SHAREDMEMORYBUFFER_SLOT_DATA]; //pkt_buffer
            pkt_buffer_enc = crypto_key_sym_encrypt(lp->k, &lp->smb.buffer[current_slot * (lp->smb.size + SHAREDMEMORYBUFFER_SLOT_META_SIZE) + SHAREDMEMORYBUFFER_SLOT_DATA], *pkt_len, &enc_len, pkt_buffer_enc);
            //std::cout << "sending enc packet: " << enc_len << std::endl;
            lp->n.send(&lp->n, pkt_buffer_enc, enc_len);
            current_slot = (current_slot + 1) % lp->smb.slots;
        } else {
            nothing_ct++;
            util_sleep(1);
            /*
            if (nothing_ct < 8) {
                util_sleep(2);
            } else if (nothing_ct < 16) {
                util_sleep(4);
            } else if (nothing_ct < 32) {
                util_sleep(8);
            } else if (nothing_ct < 64) {
                util_sleep(16);
            }           else {
                util_sleep(128);
            }
            */
        }
    }
    free(pkt_buffer);
    free(pkt_buffer_enc);
    thread_terminated(&lossy_pipes_thread_pool, lplp->thread_id);
    free(lplp);
}