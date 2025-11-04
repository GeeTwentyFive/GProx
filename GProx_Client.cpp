#include <vector>
#include <iostream>
#include <stdlib.h>
#include <string>
#include <thread>
#include <mutex>
#include <deque>

#define ENET_IMPLEMENTATION
#include "libs/enet.h"

#include <SFML/Audio.hpp>

#include "libs/InputMonitor.h"
#include "libs/AudioInputMonitor.h"


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

class AudioOutputStream : sf::SoundStream {
public:
        AudioOutputStream() {
                initialize(1, 48000, {sf::SoundChannel::Mono});
        }

        void Start() { play(); }

        void SetPosition(sf::Vector3f pos) {
                setPosition(pos);
        }

        void PushSamples(const std::vector<std::int16_t>& samples) {
                samples_mutex.lock();
                samples_queue.push_back(samples);
                samples_mutex.unlock();
        }

private:
        bool onGetData(Chunk& data) override {
                samples_mutex.lock();

                if (samples_queue.empty()) return false;

                current_samples = std::move(samples_queue.front());
                samples_queue.pop_front();

                data.samples = current_samples.data();
                data.sampleCount = current_samples.size();

                samples_mutex.unlock();
                return true;
        }

        void onSeek(sf::Time timeOffset) override {}


private:
        std::deque<std::vector<std::int16_t>> samples_queue;
        std::vector<std::int16_t> current_samples;
        std::mutex samples_mutex;
};
typedef struct {
        AudioOutputStream* audio_output_stream;
        struct in6_addr ip;
} RemotePeerAudioData;

typedef enum {
        PACKET_TYPE_AUDIO_DATA = 0,
        PACKET_TYPE_PEER_DATA = 1
} PacketType;


ENetPeer* gprox_server;
std::mutex gprox_server_mutex;
std::vector<PeerData> remote_peer_data;
std::vector<RemotePeerAudioData> remote_peer_audio_data;
std::mutex remote_peer_data_mutex;

int bRecord = 0;


void AudioInputData_Callback(const void* pData, ma_uint32 frameCount) {
        if (bRecord) {
                unsigned char* packet_data = (unsigned char*)malloc(1 + sizeof(float)*frameCount);
                packet_data[0] = PACKET_TYPE_AUDIO_DATA;
                memcpy(&packet_data[1], pData, sizeof(float)*frameCount);
                gprox_server_mutex.lock();
                enet_peer_send(
                        gprox_server,
                        0,
                        enet_packet_create(
                                packet_data,
                                1 + sizeof(float)*frameCount,
                                ENET_PACKET_FLAG_UNSEQUENCED
                        )
                );
                gprox_server_mutex.unlock();
                free(packet_data);
        }
}

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
                                if (event.packet->dataLength != (sizeof(PeerData) - sizeof(struct in6_addr))) {
                                        std::cout << "IPC ERROR: Input data size " << event.packet->dataLength << " does not match target " << (sizeof(PeerData) - sizeof(struct in6_addr)) << std::endl;
                                }

                                PeerData* local_peer_data = (PeerData*)event.packet->data;
                                sf::Listener::setPosition(sf::Vector3f{
                                        local_peer_data->pos.x,
                                        local_peer_data->pos.y,
                                        local_peer_data->pos.z
                                });

                                unsigned char* local_peer_data_packet_data = (unsigned char*)malloc(1 + sizeof(PeerData));
                                local_peer_data_packet_data[0] = PACKET_TYPE_PEER_DATA;
                                memcpy(
                                        &local_peer_data_packet_data[1],
                                        event.packet->data,
                                        sizeof(PeerData)
                                );
                                gprox_server_mutex.lock();
                                enet_peer_send(
                                        gprox_server,
                                        0,
                                        enet_packet_create(
                                                local_peer_data_packet_data,
                                                1 + sizeof(PeerData),
                                                event.packet->flags
                                        )
                                );
                                gprox_server_mutex.unlock();
                                free(local_peer_data_packet_data);
                        }
                }
        }
}

void Listen(ENetHost* local_client) {
        ENetEvent event;
        PeerData* peer_data = NULL;
        RemotePeerAudioData* peer_audio_data = NULL;
        while (true) {
                while (enet_host_service(local_client, &event, 1) > 0) {
                        if (event.type == ENET_EVENT_TYPE_RECEIVE) {
                                remote_peer_data_mutex.lock();
                                switch (event.packet->data[0]) {
                                        case PACKET_TYPE_AUDIO_DATA:
                                                peer_data = NULL;
                                                for (PeerData& p : remote_peer_data) {
                                                        if (in6_equal(p.ip, *(struct in6_addr*)&event.packet->data[event.packet->dataLength - sizeof(struct in6_addr)])) {
                                                                peer_data = &p;
                                                        }
                                                }
                                                if (peer_data == NULL) break;

                                                peer_audio_data = NULL;
                                                for (RemotePeerAudioData& a : remote_peer_audio_data) {
                                                        if (in6_equal(a.ip, *(struct in6_addr*)&event.packet->data[event.packet->dataLength - sizeof(struct in6_addr)])) {
                                                                peer_audio_data = &a;
                                                        }
                                                }
                                                if (peer_audio_data == NULL) break;

                                                peer_audio_data->audio_output_stream->SetPosition(sf::Vector3f{
                                                        peer_data->pos.x,
                                                        peer_data->pos.y,
                                                        peer_data->pos.z
                                                });

                                                // TODO: peer_audio_data->audio_output_stream->PushSamples()

                                        break;

                                        case PACKET_TYPE_PEER_DATA: // Sync remote player positions
                                                bool existing_peer_found = false;
                                                for (PeerData& p : remote_peer_data) {
                                                        if (in6_equal(p.ip, ((PeerData*)&event.packet->data[1])->ip)) {
                                                                existing_peer_found = true;
                                                                memcpy(
                                                                        &p,
                                                                        &event.packet->data[1],
                                                                        sizeof(PeerData)
                                                                );
                                                                break;
                                                        }
                                                }

                                                if (!existing_peer_found) {
                                                        PeerData p;
                                                        memcpy(
                                                                &p,
                                                                &event.packet->data[1],
                                                                sizeof(PeerData)
                                                        );
                                                        remote_peer_data.push_back(p);

                                                        AudioOutputStream* remote_peer_audio_output_stream = new AudioOutputStream;
                                                        remote_peer_audio_output_stream->Start();
                                                        remote_peer_audio_data.push_back(RemotePeerAudioData{
                                                                .audio_output_stream = remote_peer_audio_output_stream,
                                                                .ip = p.ip
                                                        });
                                                }
                                        break;
                                }
                                remote_peer_data_mutex.unlock();

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


        ENetHost* client = enet_host_create(NULL, 1, 1, 0, 0);
        if (client == NULL) {
                std::cout << "ERROR: Failed to create ENet client" << std::endl;
                exit(1);
        }

        ENetAddress address;
        enet_address_set_host_new(&address, argv[2]);
        address.port = port;
        gprox_server = enet_host_connect(client, &address, 1, 0);
        if (gprox_server == NULL) {
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
                enet_peer_reset(gprox_server);
                std::cout << "ERROR: Failed to connect to server" << std::endl;
                return 1;
        }

        // Start client listener after connecting
        std::thread(Listen, client).detach();

        // Start IPC (Inter-Process Communication) Server
        std::thread(IPC_Server, port+1).detach();


        // Bind V to push-to-talk
        if (InputMonitor__Bind(INPUTMONITOR__TARGET_KEYBOARD, VC_V, &bRecord) != 0) {
                std::cout << "ERROR: Failed to bind push-to-talk key" << std::endl;
                return 1;
        }

        if (AudioInputMonitor__Monitor(ma_format_s16, 1, 48000, AudioInputData_Callback) != 0) {
                std::cout << "ERROR: Failed to start monitoring audio input" << std::endl;
        }

        while (true) std::this_thread::sleep_for(std::chrono::milliseconds(1));

        return 0;
}