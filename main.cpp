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


#define TIMEOUT_MS 5000
#define MAX_SERVER_CLIENTS 4


void Server(enet_uint16 port) {
        std::cout << "Starting server on port " << port << std::endl;

        ENetAddress address = {ENET_HOST_ANY, port};
        ENetHost* server = enet_host_create(
                &address,
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
        while (true) {
                while (enet_host_service(server, &event, 1) > 0) {
                        if (event.type == ENET_EVENT_TYPE_RECEIVE) {
                                enet_host_broadcast(server, 0, event.packet);
                        }
                }
        }
}

void Listen(ENetHost* local_client) {
        ENetEvent event;
        while (true) {
                while (enet_host_service(local_client, &event, 1) > 0) {
                        if (event.type == ENET_EVENT_TYPE_RECEIVE) {
                                std::cout << "> " << event.packet->data << std::endl; // TEMP; TEST
                        }
                }
        }
}

int main(int argc, char* argv[]) {
        if (argc < 2) {
                std::cout << "USAGE: GProx <PORT> [SERVER_IP_ADDRESS]>" << std::endl;
                std::cout << "(If you want to host the server, enter only PORT)" << std::endl;
                return 1;
        }

        int port = atoi(argv[1]);
        if (port == 0) {
                std::cout << "ERROR: Provided port " << argv[1] << " is invalid" << std::endl;
                return 1;
        }

        std::string ip = (argc == 3) ? argv[2] : "localhost";

        if (enet_initialize() != 0) {
                std::cout << "ERROR: Failed to initialize ENet" << std::endl;
                return 1;
        }
        atexit(enet_deinitialize);


        // Start server if no IP (or "localhost") provided in CLI args
        if (ip == "localhost") std::thread(Server, (enet_uint16)port).detach();


        // Client

        ENetHost* client = enet_host_create( NULL, 1, 1, 0, 0);
        if (client == NULL) {
                std::cout << "ERROR: Failed to create ENet client" << std::endl;
                exit(1);
        }

        ENetAddress address;
        enet_address_set_host(&address, ip.c_str());
        address.port = port;
        ENetPeer* server = enet_host_connect(client, &address, 1, 0);
        if (server == NULL) {
                std::cout << "ERROR: enet_host_connect() failed" << std::endl;
                return 1;
        }

        ENetEvent event;
        if (
                enet_host_service(client, &event, TIMEOUT_MS) > 0 &&
                event.type == ENET_EVENT_TYPE_CONNECT
        ) {
                std::cout << "Connected" << std::endl;
        } else {
                enet_peer_reset(server);
                std::cout << "ERROR: Failed to connect to server" << std::endl;
                return 1;
        }

        // Start client listener after connecting
        std::thread(Listen, client).detach();

        // TEMP; TEST
        std::string test;
        while (test != "STOP") {
                std::getline(std::cin, test);
                enet_peer_send(
                        server,
                        0,
                        enet_packet_create(
                                test.c_str(),
                                test.length()+1,
                                ENET_PACKET_FLAG_UNSEQUENCED
                        )
                );
        }

        return 0;
}