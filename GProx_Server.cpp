#include <iostream>
#include <stdlib.h>

#define ENET_IMPLEMENTATION
#include "libs/enet.h"


#define MAX_CLIENTS 4


typedef struct {
        struct {
                float x;
                float y;
                float z;
        } pos;

        struct {
                float x;
                float y;
                float z;
        } rot;

        struct {
                float x;
                float y;
                float z;
        } vel;

        float volume;

// Managed by server:
        struct in6_addr ip;
} PeerData;

typedef enum {
        PACKET_TYPE_AUDIO_DATA = 0,
        PACKET_TYPE_PEER_DATA = 1
} PacketType;


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
        size_t initial_packet_size = 0;
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
                                        switch (event.packet->data[0]) {
                                                        // Put ip at end of audio data
                                                        case PACKET_TYPE_AUDIO_DATA:
                                                                initial_packet_size = event.packet->dataLength;
                                                                enet_packet_resize(event.packet, initial_packet_size + sizeof(struct in6_addr));
                                                                memcpy(
                                                                        &event.packet->data[initial_packet_size],
                                                                        &event.peer->address.host,
                                                                        sizeof(struct in6_addr)
                                                                );
                                                        break;

                                                        // Copy ip into PeerData
                                                        case PACKET_TYPE_PEER_DATA:
                                                                PeerData* d = (PeerData*)event.packet->data;
                                                                memcpy(&d->ip, &event.peer->address.host, sizeof(struct in6_addr));
                                                        break;
                                                }

                                        // Broadcast to all but sender
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