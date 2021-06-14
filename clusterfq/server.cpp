#include "server.h"

#include "network.h"
#include "mutex.h"
#include "util.h"
#include "message.h"
#include "address_factory.h"
#include "identity.h"
#include "contact.h"
#include "crypto.h"

#include <iostream>
#ifdef _WIN32

#else
#include <cstring>
#endif

struct Network udp_multicast_server;

unsigned char*  packet_buffer            = nullptr;
unsigned int    packet_buffer_write_slot = 0;
unsigned int    packet_buffer_read_slot  = SERVER_PACKET_BUFFER_SLOTS; // - 1 // temp. unused

struct mutex    packet_buffer_mutex;

void server_init() {
    mutex_init(&packet_buffer_mutex);
    packet_buffer = (unsigned char*)malloc(((45 + 1) + (SERVER_MAX_PACKET_SIZE + 1)) * SERVER_PACKET_BUFFER_SLOTS);

	network_init(&udp_multicast_server);
	network_udp_multicast_socket_server_create(&udp_multicast_server, 1337);
}

void server_group_join(string address) {
	network_udp_multicast_socket_server_group_join(&udp_multicast_server, address.c_str());
}

void server_group_leave(string address) {
	network_udp_multicast_socket_server_group_drop(&udp_multicast_server, address.c_str());
}

void server_wait_for_next_write_slot() {
    while (true) {
        mutex_wait_for(&packet_buffer_mutex);
        if ((packet_buffer_write_slot + 1) % SERVER_PACKET_BUFFER_SLOTS != packet_buffer_read_slot) {
            packet_buffer_write_slot = (packet_buffer_write_slot + 1) % SERVER_PACKET_BUFFER_SLOTS;
            break;
        }
        mutex_release(&packet_buffer_mutex);
        util_sleep(16);
    }
    mutex_release(&packet_buffer_mutex);
}

unsigned char *server_wait_for_next_read_slot(unsigned int interval_milliseconds) {
    while (true) {
        mutex_wait_for(&packet_buffer_mutex);
        if ((packet_buffer_read_slot + 1) % SERVER_PACKET_BUFFER_SLOTS != packet_buffer_write_slot) {
            packet_buffer_read_slot = (packet_buffer_read_slot + 1) % SERVER_PACKET_BUFFER_SLOTS;
            break;
        }
        mutex_release(&packet_buffer_mutex);
        util_sleep(interval_milliseconds);
    }
    mutex_release(&packet_buffer_mutex);
    return &packet_buffer[packet_buffer_read_slot * ((45 + 1) + (SERVER_MAX_PACKET_SIZE + 1))];
}

void server_loop(void* param) {
    unsigned char* packet_buffer_dst_addr = packet_buffer;
    unsigned char* packet_buffer_packet = packet_buffer_dst_addr + (45 + 1);
    while (true) {
        memset(packet_buffer_dst_addr, 0, (45 + 1) + (SERVER_MAX_PACKET_SIZE + 1));

        packet_buffer_dst_addr[45] = '\0';
        packet_buffer_packet[SERVER_MAX_PACKET_SIZE] = '\0';

        unsigned int out_len = 0;

        udp_multicast_server.read(&udp_multicast_server, packet_buffer_packet, SERVER_MAX_PACKET_SIZE, (char*)packet_buffer_dst_addr, &out_len);
        
        string dst_addr((char *)packet_buffer_dst_addr);

        vector<struct address_factory_sender> senders = address_factory_sender_get(dst_addr);

        bool packet_was_processed = false;

        for (int s = 0; s < senders.size(); s++) {
            struct address_factory_sender* afs = &senders[s];
            if (afs->afst == AFST_CONTACT) {
                struct identity* i = identity_get(afs->identity_id);
                struct contact* con = contact_get(&i->contacts, afs->sender_id);

                if (contact_process_message(i, con, packet_buffer_packet, out_len)) {
                    //message was from contact/could be processed
                    packet_was_processed = true;
                    break;
                } else {
                    //message was not from contact/could not be processed
                }
            }
        }

        if (packet_was_processed) {
            //sending receipts
            packetset_loop_start_if_needed();
        }

        server_wait_for_next_write_slot();
        
        packet_buffer_dst_addr = &packet_buffer[packet_buffer_write_slot * ((45 + 1) + (SERVER_MAX_PACKET_SIZE + 1))];
        packet_buffer_packet = packet_buffer_dst_addr + (45 + 1);
    }
}