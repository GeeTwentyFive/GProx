g++ -O3 ../GProx_Server.cpp -lws2_32 -lwinmm -o GProx_Server.exe
g++ -O3 ../GProx_Client.cpp -lws2_32 -lwinmm ../libs/libuiohook/libuiohook_windows.a -I../libs ../libs/SFML/libsfml-audio-s_windows.a ../libs/SFML/libsfml-system-s_windows.a -o GProx_Client.exe
