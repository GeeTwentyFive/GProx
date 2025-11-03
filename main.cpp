#include <iostream>
#include <stdlib.h>
#include <string>
#include <thread>

#define ENET_IMPLEMENTATION
#include "libs/enet.h"

#define MINIAUDIO_IMPLEMENTATION
#include "libs/miniaudio.h"

#define WITH_MINIAUDIO
#include "libs/soloud/include/soloud.h"
#include "libs/soloud/include/soloud_wavstream.h"


#define MAX_SERVER_CLIENTS 4


void Server(enet_uint16 port) {
        ENetHost* server = enet_host_create(
                new ENetAddress {ENET_HOST_ANY, port},
                MAX_SERVER_CLIENTS,
                1,
                0,
                0
        );
        if (server == NULL) {
                std::cout << "ERROR: Failed to create ENet server" << std::endl;
                exit(1);
        }

        ENetEvent event;
        while (enet_host_service(server, &event, 0) > 0) {
                if (event.type == ENET_EVENT_TYPE_RECEIVE) {
                        enet_host_broadcast(server, 0, event.packet);
                        enet_packet_destroy(event.packet);
                }
        }
}


int main(int argc, char* argv[]) {
        for (int i = 0; i < 2; i++) {
                std::cout << "USAGE: GProx <PORT> [SERVER_IPv6_ADDRESS]>" << std::endl;
                std::cout << "(If you want to host the server, enter only PORT)" << std::endl;
                return 1;
        }

        int port = atoi(argv[1]);
        if (port == 0) {
                std::cout << "ERROR: Provided port " << argv[1] << " is invalid" << std::endl;
                return 1;
        }

        std::string ip = (argc == 3) ? argv[2] : "::1";

        if (enet_initialize() != 0) {
                std::cout << "ERROR: Failed to initialize ENet" << std::endl;
                return 1;
        }
        atexit(enet_deinitialize);

        // TODO

        return 0;
}