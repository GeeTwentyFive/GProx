GProx is a general-purpose cross-platform proximity voice chat client and server. The user submits 3D positional data to their local GProx client via local network IPC (Inter-Process Communication). This does have a downside of adding less than 1ms of latency, but provides the HUGE benefit of super simple integration into any engine, new application, and/or existing application in any language which provides networking capabilities (so like almost all of them).

# Usage
1) One of the users starts server via `GProx_Server <PORT>`
2) All users connect to the server via `GProx_Client <LOCAL_IPC_PORT> <SERVER_PORT> <SERVER_IPv6_ADDRESS>`
3) Local user sends their local player data via TCP (in Least Significant Bit order) to `localhost:<LOCAL_IPC_PORT>`, format:
```
typedef struct {
        struct {
                float x; // 4 bytes
                float y; // 4 bytes
                float z; // 4 bytes
        } pos;

        struct {
                float x; // 4 bytes
                float y; // 4 bytes
                float z; // 4 bytes
        } rot;

        struct {
                float x; // 4 bytes
                float y; // 4 bytes
                float z; // 4 bytes
        } vel;

        float volume; // 4 bytes
} PeerData;
```
^ total size: 40 bytes