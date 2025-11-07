GProx is a general-purpose cross-platform proximity voice chat client and server. The user submits 3D positional data to their local GProx client via local network IPC (Inter-Process Communication). This does have a downside of adding less than 1ms of latency, but provides the HUGE benefit of super simple integration into any engine, new application, and/or existing application in any language which provides networking capabilities (so like almost all of them).

# Usage
1) One of the users starts server via `GProx_Server <PORT>`
2) All users connect to the server via `GProx_Client <LOCAL_IPC_PORT> <SERVER_PORT> <SERVER_IPv6_ADDRESS>`
3) User sends their local player data data via ENet to `localhost:<LOCAL_IPC_PORT>`, format: `f32 pos_x, f32 pos_y, f32 pos_z, f32 dir_x, f32 dir_y, f32 dir_z, f32 vel_x, f32 vel_y, f32 vel_z, f32 volume`
