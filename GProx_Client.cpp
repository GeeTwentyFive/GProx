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


#define CONNECT_TIMEOUT_MS 5000


typedef struct {
        struct in6_addr ip;
        struct {
                float x;
                float y;
                float z;
        } pos;
} PeerData;

typedef enum {
        PACKET_TYPE_AUDIO_DATA = 0,
        PACKET_TYPE_PEER_DATA = 1
} PacketType;


void Listen(ENetHost* local_client) {
        ENetEvent event;
        while (true) {
                while (enet_host_service(local_client, &event, 1) > 0) {
                        switch (event.type) {
                                case ENET_EVENT_TYPE_RECEIVE:
                                        std::cout << "> " << event.packet->data << std::endl; // TEMP; TEST
                                break;

                                default: break;
                        }
                }
        }
}

int main(int argc, char* argv[]) {
        if (argc < 3) {
                std::cout << "USAGE: GProx_Client <PORT> <SERVER_IPv6_ADDRESS>" << std::endl;
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


        ENetHost* client = enet_host_create(NULL, 1, 1, 0, 0);
        if (client == NULL) {
                std::cout << "ERROR: Failed to create ENet client" << std::endl;
                exit(1);
        }

        ENetAddress address;
        enet_address_set_host_new(&address, argv[2]);
        address.port = port;
        ENetPeer* server = enet_host_connect(client, &address, 1, 0);
        if (server == NULL) {
                std::cout << "ERROR: enet_host_connect() failed" << std::endl;
                return 1;
        }

        ENetEvent event;
        if (
                enet_host_service(client, &event, CONNECT_TIMEOUT_MS) > 0 &&
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