// clusterfq_elevated.cpp : Diese Datei enthält die Funktion "main". Hier beginnt und endet die Ausführung des Programms.
//

#include <string>
#include <sstream>
#include <iostream>

#ifdef _WIN32
#else
#include <cstring>
#endif

using namespace std;

void network_address_add(string interface, string address, string prefix_len) {
    stringstream cmd;
#ifdef _WIN32
    cmd << "netsh interface ipv6 add address interface=" << interface << " address=" << address << " type=unicast validlifetime=infinite preferredlifetime=infinite store=active skipassource=true";
#else
    cmd << "sudo /sbin/ip -6 addr add " << address << "/" << prefix_len << " dev " << interface;
#endif
    std::cout << cmd.str() << std::endl;
    system(cmd.str().c_str());
}

void network_address_delete(string interface, string address, string prefix_len) {
    stringstream cmd;
#ifdef _WIN32
    cmd << "netsh interface ipv6 delete address interface=" << interface << " address=" << address << " store=active";
#else
    cmd << "sudo /sbin/ip -6 addr del " << address << "/" << prefix_len << " dev " << interface;
#endif
    std::cout << cmd.str() << std::endl;
    system(cmd.str().c_str());
}

int main(int argc, char *argv[]) {
    if (argc == 5) {
        if (strstr(argv[1], "add_address") != nullptr) {
            network_address_add(string(argv[2]), string(argv[3]), string(argv[4]));
        } else if (strstr(argv[1], "delete_address") != nullptr) {
            network_address_delete(string(argv[2]), string(argv[3]), string(argv[4]));
        }
    }
    return 0;
}
