#include <vector>
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
        struct {
                float x;
                float y;
                float z;
        } pos;

// Managed by server:
        struct in6_addr ip;
} PeerData;

typedef enum {
        PACKET_TYPE_AUDIO_DATA = 0,
        PACKET_TYPE_PEER_DATA = 1
} PacketType;


PeerData local_peer_data = {0};
std::vector<PeerData> remote_peer_data;


void IPC_Server(enet_uint16 port) {
        ENetAddress address;
        address.host = ENET_HOST_ANY;
        address.port = port;
        ENetHost* server = enet_host_create(
                &address,
                1,
                1,
                0,
                0
        );
        if (server == NULL) {
                std::cout << "ERROR: Failed to start IPC server" << std::endl;
                exit(1);
        }

        std::cout << "Started IPC server on port " << port << std::endl;

        ENetEvent event;
        while (true) {
                while (enet_host_service(server, &event, 1) > 0) {
                        if (event.type == ENET_EVENT_TYPE_RECEIVE) {
                                if (event.packet->dataLength != sizeof(PeerData)) {
                                        std::cout << "IPC ERROR: Input data size " << event.packet->dataLength << " does not match target " << sizeof(PeerData) << std::endl;
                                }
                                memcpy(
                                        &local_peer_data,
                                        event.packet->data,
                                        event.packet->dataLength
                                );
                                enet_packet_destroy(event.packet);
                        }
                }
        }
}

void Listen(ENetHost* local_client) {
        ENetEvent event;
        while (true) {
                while (enet_host_service(local_client, &event, 1) > 0) {
                        if (event.type == ENET_EVENT_TYPE_RECEIVE) {
                                switch (event.packet->data[0]) {
                                        case PACKET_TYPE_AUDIO_DATA:
                                                PeerData* peer_data = NULL;
                                                for (PeerData& p : remote_peer_data) {
                                                        if (in6_equal(p.ip, ((PeerData*)&event.packet->data[1])->ip)) {
                                                                peer_data = &p;
                                                        }
                                                }
                                                if (peer_data == NULL) break;

                                                // TODO: Play based on position in peer_data
                                        break;

                                        case PACKET_TYPE_PEER_DATA: // Sync remote player positions
                                                for (PeerData& p : remote_peer_data) {
                                                        if (in6_equal(p.ip, ((PeerData*)&event.packet->data[1])->ip)) {
                                                                memcpy(
                                                                        &p,
                                                                        &event.packet->data[1],
                                                                        sizeof(PeerData)
                                                                );
                                                                break;
                                                        }
                                                }

                                                PeerData p;
                                                memcpy(
                                                        &p,
                                                        &event.packet->data[1],
                                                        sizeof(PeerData)
                                                );
                                                remote_peer_data.push_back(p);
                                        break;
                                }

                                enet_packet_destroy(event.packet);
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


        // Start IPC (Inter-Process Communication) Server
        std::thread(IPC_Server, port+1).detach();


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

        // TODO: Record -> compress -> send

        return 0;
}