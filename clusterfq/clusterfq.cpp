#include "clusterfq.h"

#include <iostream>

#include "crypto.h"
#include "network.h"
#include "identity.h"
#include "shell.h"
#include "mutex.h"
#include "address_factory.h"
#include "server.h"
#include "client.h"
#include "util.h"
#include "message.h"
#include "clusterfq.h"

#include <string>
#include <vector>
#include <sstream>

#ifdef _WIN32
#include <ws2def.h>
#else
#include <cstring>
#endif

using namespace std;

struct ThreadPool   main_thread_pool;

int main(int argc, char **argv) {
    thread_pool_init(&main_thread_pool, 5);

    address_factory_init();
    contact_static_init();
    packetset_static_init();

    /* START SERVER */
    server_init();
    thread_create(&main_thread_pool, (void*)&server_loop, nullptr);
    /* ------------ */

    /* INIT CLIENT */
    client_init();
    /* ----------- */

    /* START SHELL */
    thread_create(&main_thread_pool, (void *)&shell_loop, nullptr);
    /* ----------- */

    /* PROCESS SHELL COMMANDS */
    while (true) {
        mutex_wait_for(&shell_cmd_lock);
        for (int sc = 0; sc < shell_cmd_queue.size(); sc++) {
            std::vector<std::string> args = shell_cmd_queue[sc];
            if (strstr(args[0].c_str(), "identity_create") != nullptr) {
                struct identity i;
                identity_create(&i, args[1]);
                identities.push_back(i);
            } else if (strstr(args[0].c_str(), "identity_load") != nullptr) {
                identity_load(stoi(args[1]));
            } else if (strstr(args[0].c_str(), "identities_load") != nullptr) {
                identities_load();
            } else if (strstr(args[0].c_str(), "identities_list") != nullptr) {
                identities_list();
            } else if (strstr(args[0].c_str(), "identity_contact_add") != nullptr) {
                struct contact c;
                contact_create(&c, args[2], args[3], args[4]);
                identity_contact_add(stoi(args[1]), &c);
            } else if (strstr(args[0].c_str(), "identity_share") != nullptr) {
                identity_share(stoi(args[1]), args[2]);
            } else if (strstr(args[0].c_str(), "identity_contact_list") != nullptr) {
                identity_contact_list(stoi(args[1]));
            } else if (strstr(args[0].c_str(), "identity_migrate_key") != nullptr) {
                struct identity* i = identity_get(stoi(args[1]));
                identity_migrate_key(i, stoi(args[2]));
            } else if (strstr(args[0].c_str(), "address_factory_sender_get") != nullptr) {
                vector<struct address_factory_sender> afs = address_factory_sender_get(args[1]);
                    std::cout << std::endl;
                    for (int a = 0; a < afs.size(); a++) {
                        std::cout << afs[a].afst << std::endl;
                        std::cout << afs[a].identity_id << std::endl;
                        std::cout << afs[a].sender_id << std::endl;
                    }
                    std::cout << "/----------------/" << std::endl;
            } else if (strstr(args[0].c_str(), "address_factory_clear") != nullptr) {
                address_factory_clear();
            } else if (strstr(args[0].c_str(), "message_send_file") != nullptr) {
                message_send_file(stoi(args[1]), stoi(args[2]), (unsigned char*)args[3].c_str(), strlen(args[3].c_str()));
            } else if (strstr(args[0].c_str(), "message_send") != nullptr) {
                message_send(stoi(args[1]), stoi(args[2]), (unsigned char*)args[3].c_str(), strlen(args[3].c_str()));
            } else if (strstr(args[0].c_str(), "contact_save") != nullptr) {
                struct identity* i = identity_get(stoi(args[1]));
                struct contact* c = contact_get(&i->contacts, stoi(args[2]));
                stringstream cp;
                cp << "./identities/" << i->id << "/contacts/" << c->id << "/";
                contact_save(c, cp.str());
            } else if (strstr(args[0].c_str(), "contact_stats_dump") != nullptr) {
                struct identity *i = identity_get(stoi(args[1]));
                struct contact* c = contact_get(&i->contacts, stoi(args[2]));
                contact_stats_dump(c->cs, stoi(args[3]), stoi(args[4]));
            } 
        }
        shell_cmd_queue.clear();
        mutex_release(&shell_cmd_lock);
        util_sleep(1000);
    }
    /* ---------------------- */

    /* TEST ENCRYPTION/DECRYPTION */
    /*
    struct Key k1, k2, s1, s2;
    std::cout << "generating key\n";

    crypto_key_private_generate(&k1, 2048);
    
    std::cout << k1.private_key << std::endl;

    std::cout << "extracting pubkey\n";

    crypto_key_public_extract(&k1);

    std::cout << k1.public_key << std::endl;

    string testmessage = "Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!Hello World!";
    std::cout << "testmsg len " << testmessage.length() << std::endl; 

    char* signature = crypto_sign_message(&k1, (char *)testmessage.c_str(), testmessage.length());
    bool verified = crypto_verify_signature(&k1, (char*)testmessage.c_str(), testmessage.length(), signature, strlen(signature));

    if (verified) {
        std::cout << "message verified k1\n";
    }

    crypto_key_private_generate(&k2, 2048);
    crypto_key_public_extract(&k2);

    bool verified_2 = crypto_verify_signature(&k2, (char*)testmessage.c_str(), testmessage.length(), signature, strlen(signature));
    
    if (verified_2) {
        std::cout << "message verified k2\n";
    }

    std::cout << "encrypting message\n";

    unsigned int out_len_pe = 0;
    char *encrypted = crypto_key_public_encrypt(&k1, (char *)testmessage.c_str(), testmessage.length(), &out_len_pe);

    std::cout << encrypted <<"\n";

    std::cout << "decrypting message\n";

    char* decrypted = crypto_key_private_decrypt(&k1, encrypted, out_len_pe);

    std::cout << decrypted << std::endl;

    crypto_key_sym_generate(&s1);
    crypto_key_name_set(&s1, "s1", 2);

    crypto_key_dump(&s1);

    crypto_key_sym_generate(&s2);
    crypto_key_name_set(&s2, "s2", 2);

    crypto_key_dump(&s2);

    char* tmp = (char*)malloc(s1.public_key_len);
    memcpy(tmp, s1.public_key, s1.public_key_len);

    memcpy(s1.public_key, s2.public_key, s2.public_key_len);
    memcpy(s2.public_key, tmp, s2.public_key_len);

    crypto_key_sym_finalise(&s1);
    crypto_key_sym_finalise(&s2);

    crypto_key_dump(&s1);
    crypto_key_dump(&s2);

    int out_len = 0;
    unsigned char *encrypted_2 = crypto_key_sym_encrypt(&s1, (unsigned char *)testmessage.c_str(), testmessage.length(), &out_len);
    int out_len_2 = 0;
    unsigned char *decrypted_2 = crypto_key_sym_decrypt(&s1, encrypted_2, out_len, &out_len_2);

    int out_len_3 = 0;
    unsigned char* decrypted_3 = crypto_key_sym_decrypt(&s2, encrypted_2, out_len, &out_len_3);

    std::cout << encrypted_2 << std::endl;
    std::cout << decrypted_2 << std::endl;
    std::cout << decrypted_3 << std::endl;
    */
    /* -------------------------- */

    /* TEST NETWORK */
    /*
    struct Network n, n2;

    char* packet_data = (char *)malloc(100);

    if (argc == 2) {
        network_init(&n);
        network_udp_multicast_socket_server_create(&n, 1337);
        network_udp_multicast_socket_server_group_join(&n, "ff12::1337:1337:1337:1337");
        network_udp_multicast_socket_server_group_join(&n, "ff12::1337:1337:1337:1338");

        while (true) {
            memset(packet_data, 0, 100);

            char dst_address[46];
            dst_address[45] = '\0';

            n.read(&n, packet_data, 100, (char *)&dst_address);

            std::cout << packet_data << std::endl;
            std::cout << dst_address << std::endl;
        }
    } else {
        network_init(&n);
        network_udp_multicast_socket_client_create(&n, "ff12::1337:1337:1337:1337", 1337);

        network_init(&n2);
        network_udp_multicast_socket_client_create(&n2, "ff12::1337:1337:1337:1338", 1337);

        for (char i = 0; i < 10; i++) {
            memset(packet_data, 0, 10);
            packet_data[0] = 48 + i;
           n.send(&n, packet_data, 10);
        }

        for (char i = 0; i < 10; i++) {
            memset(packet_data, 0, 10);
            packet_data[0] = 58 + i;
            n2.send(&n2, packet_data, 10);
        }
    }  
    */
    /* ------------ */

    /* TEST UNAMBIGUOUS IPV6 ADDRESS REPRESENTATION */
    /*
    char pbar[46];

    string tmp("ff12::1337:f22:4733:a400");

    snprintf((char *)&pbar, 46, "%s", tmp.c_str());

    address_factory_fix_address_rfc((char *)&pbar);
    std::cout << pbar << std::endl;

    string tmp2("ff12::1337:f22:43:a400");

    snprintf((char*)&pbar, 46, "%s", tmp2.c_str());

    address_factory_fix_address_rfc((char*)&pbar);
    std::cout << pbar << std::endl;

    string tmp3("ff12::1337:0:43:a400");

    snprintf((char*)&pbar, 46, "%s", tmp3.c_str());

    address_factory_fix_address_rfc((char*)&pbar);
    std::cout << pbar << std::endl;

    string tmp4("ff12:0000:1234:3214:1337:0:43:a400");

    snprintf((char*)&pbar, 46, "%s", tmp4.c_str());

    address_factory_fix_address_rfc((char*)&pbar);
    std::cout << pbar << std::endl;
    */
    /* -------------------------------------------- */
}