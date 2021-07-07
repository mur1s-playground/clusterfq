// clusterfq_shell.cpp : Diese Datei enthält die Funktion "main". Hier beginnt und endet die Ausführung des Programms.
//

#include <iostream>

#include "../clusterfq/thread.h"
#include "../clusterfq/util.h"

#include "shell.h"

#include "../clusterfq_cl/clusterfq_cl.h"
#include "../clusterfq_cl/const_defs.h"

struct ThreadPool main_thread_pool;

int main() {
    thread_pool_init(&main_thread_pool, 5);

    /* START SHELL */
    thread_create(&main_thread_pool, (void *)&shell_loop, nullptr);
    /* ----------- */

    ClusterFQ::ClusterFQ::init("::1", 8080);

    /* PROCESS SHELL COMMANDS */
    while (true) {
        mutex_wait_for(&shell_cmd_lock);
        for (int sc = 0; sc < shell_cmd_queue.size(); sc++) {
            std::vector<std::string> args = shell_cmd_queue[sc];

            string response = "";

            if (strstr(args[0].c_str(), "identity_create") != nullptr) {
                const char* module = ClusterFQ::MODULE_IDENTITY_POST;
                const char* module_action = ClusterFQ::IDENTITY_POST_ACTION_CREATE;
                int ps_id = ClusterFQ::ClusterFQ::paramset_create();
                ClusterFQ::ClusterFQ::paramset_param_add(ps_id, ClusterFQ::IDENTITY_POST_ACTION_CREATE_PARAM_NAME, args[1]);
                response = ClusterFQ::ClusterFQ::query(module, module_action, ps_id);
            } else if (strstr(args[0].c_str(), "identity_load_all") != nullptr) {
                const char* module = ClusterFQ::MODULE_IDENTITY_POST;
                const char* module_action = ClusterFQ::IDENTITY_POST_ACTION_LOAD_ALL;
                int ps_id = -1;
                response = ClusterFQ::ClusterFQ::query(module, module_action, ps_id);
            } else if (strstr(args[0].c_str(), "identities_list") != nullptr) {
                const char* module = ClusterFQ::MODULE_IDENTITY_GET;
                const char* module_action = ClusterFQ::IDENTITY_GET_ACTION_LIST;
                int ps_id = -1;
                response = ClusterFQ::ClusterFQ::query(module, module_action, ps_id);
            } else if (strstr(args[0].c_str(), "identity_contact_add") != nullptr) {
               
            } else if (strstr(args[0].c_str(), "identity_share") != nullptr) {
                
            } else if (strstr(args[0].c_str(), "identity_contact_list") != nullptr) {
                
            } else if (strstr(args[0].c_str(), "identity_migrate_key") != nullptr) {
                
            } else if (strstr(args[0].c_str(), "identity_remove_obsolete_keys") != nullptr) {
                
            } else if (strstr(args[0].c_str(), "address_factory_sender_get") != nullptr) {
               
            } else if (strstr(args[0].c_str(), "address_factory_clear") != nullptr) {
                
            } else if (strstr(args[0].c_str(), "message_send_file") != nullptr) {
                
            } else if (strstr(args[0].c_str(), "message_send") != nullptr) {
                const char* module = ClusterFQ::MODULE_MESSAGE_POST;
                const char* module_action = ClusterFQ::MESSAGE_POST_ACTION_SEND;
                int ps_id = ClusterFQ::ClusterFQ::paramset_create();
                ClusterFQ::ClusterFQ::paramset_param_add(ps_id, ClusterFQ::MESSAGE_POST_ACTION_SEND_PARAM_IDENTITY_ID, stoi(args[1]));
                ClusterFQ::ClusterFQ::paramset_param_add(ps_id, ClusterFQ::MESSAGE_POST_ACTION_SEND_PARAM_CONTACT_ID, stoi(args[2]));
                ClusterFQ::ClusterFQ::paramset_param_add(ps_id, ClusterFQ::MESSAGE_POST_ACTION_SEND_PARAM_TYPE, string(ClusterFQ::PARAM_TYPE_VALUE_TEXT));
                ClusterFQ::ClusterFQ::paramset_param_add(ps_id, ClusterFQ::MESSAGE_POST_ACTION_SEND_PARAM_MESSAGE, args[3]);
                response = ClusterFQ::ClusterFQ::query(module, module_action, ps_id);
            } else if (strstr(args[0].c_str(), "contact_save") != nullptr) {
                
            } else if (strstr(args[0].c_str(), "contact_stats_dump") != nullptr) {
                
            } else if(strstr(args[0].c_str(), "contact_get_chat") != nullptr) {
                const char* module = ClusterFQ::MODULE_CONTACT_GET;
                const char* module_action = ClusterFQ::CONTACT_GET_ACTION_CHAT;
                int ps_id = ClusterFQ::ClusterFQ::paramset_create();
                ClusterFQ::ClusterFQ::paramset_param_add(ps_id, ClusterFQ::CONTACT_GET_ACTION_CHAT_PARAM_IDENTITY_ID, stoi(args[1]));
                ClusterFQ::ClusterFQ::paramset_param_add(ps_id, ClusterFQ::CONTACT_GET_ACTION_CHAT_PARAM_CONTACT_ID, stoi(args[2]));
                ClusterFQ::ClusterFQ::paramset_param_add(ps_id, ClusterFQ::CONTACT_GET_ACTION_CHAT_PARAM_TIME_START, stoll(args[3]));
                ClusterFQ::ClusterFQ::paramset_param_add(ps_id, ClusterFQ::CONTACT_GET_ACTION_CHAT_PARAM_TIME_END, stoll(args[4]));
                response = ClusterFQ::ClusterFQ::query(module, module_action, ps_id);
            }
            std::cout << response;
        }
        shell_cmd_queue.clear();
        mutex_release(&shell_cmd_lock);
        util_sleep(1000);
    }
    /* ---------------------- */
}