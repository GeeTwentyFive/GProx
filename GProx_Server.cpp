#include <iostream>
#include <stdlib.h>

#define ENET_IMPLEMENTATION
#include "libs/enet.h"


#define MAX_CLIENTS 4


int main(int argc, char* argv[]) {
        if (argc < 2) {
                std::cout << "USAGE: GProx_Server <PORT>" << std::endl;
                return 1;
        }

        int port = atoi(argv[1]);
        if (port == 0) {
                std::cout << "ERROR: Provided port " << argv[1] << " is invalid" << std::endl;
                return 1;
        }


        if (enet_initialize() != 0) {
                std::cout << "ERROR: Failed to initialize ENet" << std::endl;
                return 1;
        }
        atexit(enet_deinitialize);


        std::cout << "Starting server on port " << port << std::endl;

        ENetAddress address;
        address.host = ENET_HOST_ANY;
        address.port = port;
        ENetHost* server = enet_host_create(
                &address,
                MAX_CLIENTS,
                1,
                0,
                0
        );
        if (server == NULL) {
                std::cout << "ERROR: Failed to create ENet server" << std::endl;
                return 1;
        }

        std::cout << "Server started" << std::endl;

        ENetEvent event;
        char addr_str[64] = {0};
        while (true) {
                while (enet_host_service(server, &event, 1) > 0) {
                        switch (event.type) {
                                case ENET_EVENT_TYPE_CONNECT:
                                        inet_ntop(
                                                AF_INET6,
                                                &event.peer->address.host,
                                                addr_str,
                                                sizeof(addr_str)
                                        );
                                        std::cout << "Client " << addr_str << " connected" << std::endl;
                                break;

                                case ENET_EVENT_TYPE_RECEIVE:
                                        for (
                                                ENetPeer* currentPeer = server->peers;
                                                currentPeer < &server->peers[server->peerCount];
                                                ++currentPeer
                                        ) {
                                                if (currentPeer->state != ENET_PEER_STATE_CONNECTED) continue;
                                                if (currentPeer == event.peer) continue;

                                                enet_peer_send(currentPeer, 0, event.packet);
                                        }
                                break;

                                case ENET_EVENT_TYPE_DISCONNECT:
                                case ENET_EVENT_TYPE_DISCONNECT_TIMEOUT:
                                        inet_ntop(
                                                AF_INET6,
                                                &event.peer->address.host,
                                                addr_str,
                                                sizeof(addr_str)
                                        );
                                        std::cout << "Client " << addr_str << " disconnected" << std::endl;
                                break;

                                default: break;
                        }
                }
        }

        return 0;
}