#ifndef _LOCAL_IPC_HPP
#define _LOCAL_IPC_HPP


#include <cstdint>
#include <vector>
#include <iostream>
#include <string>

#if defined(__linux__) || defined(unix)
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#elif defined(_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>

#typedef int socklen_t
#typedef long int ssize_t

#endif


namespace LocalIPC {
        namespace {
                static inline void Error(const char* msg) {
                        throw std::runtime_error(
                                std::string("ERROR: ") + msg
#if defined(__linux__) || defined(unix)
                                + ", with errno: " + std::to_string(errno)

#elif defined(_WIN32)
                                + ", with WSAGetLastError() code: " + std::to_string(WSAGetLastError())

#endif
                        );
                }
        }



        class _Common {

        protected:
                int sock;
                struct sockaddr_in address = {0};



        public:
                _Common(uint16_t port) {
#if defined(_WIN32)
                        WSADATA wsaData;
                        int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
                        if (result != 0) throw std::runtime_error(
                                std::string("ERROR: Failed to initialize Winsock, with WSAGetLastError() code: ") + std::to_string(WSAGetLastError())
                        );
#endif

                        this->sock = socket(AF_INET, SOCK_STREAM, 0);
#if defined(_WIN32)
                        if (this->sock == INVALID_SOCKET)
#else
                        if (this->sock == -1)
#endif
                                Error("Failed to create socket");

                        address.sin_family = AF_INET;
                        address.sin_port = htons(port);
                        address.sin_addr.s_addr = inet_addr("127.0.0.1");
                }



                virtual ~_Common() {
#if defined(__linux__) || defined(unix)
                        close(this->sock);

#elif defined(_WIN32)
                        closesocket(this->sock);

#endif
                }



                //
                // Returns number of bytes sent, or -1 on error
                //
                int Send(std::vector<uint8_t> data) {
                        data.insert(data.begin(), {
                                (uint8_t)((data.size() >> 0) & 0xFF),
                                (uint8_t)((data.size() >> 8) & 0xFF),
                                (uint8_t)((data.size() >> 16) & 0xFF),
                                (uint8_t)((data.size() >> 24) & 0xFF)
                        });
                        return send(this->sock, (const char*)data.data(), data.size(), 0);
                }



                //
                // Returns empty vector on error
                //
                std::vector<uint8_t> Recv() {
                        ssize_t amount_read = 0;

                        int data_size = 0;
                        amount_read = recv(this->sock, &data_size, sizeof(int), 0);
                        if (amount_read != sizeof(int)) return std::vector<uint8_t>();

                        std::vector<uint8_t> data;
                        data.resize(data_size);

                        amount_read = recv(this->sock, data.data(), data_size, MSG_WAITALL);
                        if (amount_read != data_size) return std::vector<uint8_t>();

                        return data;
                }



        };



        class Server : public _Common {
        public:
                Server(uint16_t port) : _Common(port) {
                        int result;

                        result = bind(sock, (struct sockaddr*)&address, sizeof(address));
                        if (result == -1) Error("Failed to bind server to provided port");

                        result = listen(sock, 1);
                        if (result == -1) Error("listen() failed on server");

                        socklen_t address_len = sizeof(address);
                        sock = accept(sock, (struct sockaddr*)&address, &address_len);
                }
        };



        class Client : public _Common {
        public:
                Client(uint16_t port) : _Common(port) {
                        int result;

                        result = connect(sock, (struct sockaddr*)&address, sizeof(address));
                        if (result == -1) Error("Failed to connect to provided port on local host");
                }
        };
}


#endif // _LOCAL_IPC_HPP
