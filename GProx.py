import sys
import ipaddress
from typing import Any
import socket
import threading
import pyaudio
import pyogg
import openal
import atexit


MAX_CLIENTS = 8
MAX_PACKET_SIZE = 65536


if len(sys.argv) < 2:
        print("USAGE: GProx <PORT> [SERVER_IPv6_ADDRESS]>")
        print("(If you want to host the server, enter only PORT)")
        exit(1)


try:
        port = int(sys.argv[1])
except Exception as e:
        print(f"ERROR: Provided port {sys.argv[1]} is invalid")
        print(e)
        exit(1)

ip = "::1"
is_server = True
if len(sys.argv) == 3:
        is_server = False
        try:
                ipaddress.IPv6Address(sys.argv[2])
        except Exception as e:
                print(f"ERROR: Provided IPv6 address {sys.argv[2]} is invalid")
                print(e)
                exit(1)
        ip = sys.argv[2]


Address = str


# Server
def server(port):
        clients: dict[Address, Any] = {}

        sock = socket.socket(socket.AF_INET6, socket.SOCK_DGRAM)
        sock.setblocking(False)
        sock.bind(("", port))
        while True:
                sock.listen(MAX_CLIENTS)
                try:
                        # On connect:
                        conn, addr = sock.accept()
                        clients[addr]["conn"] = conn
                        clients[addr]["pos"] = {"x": 0, "y": 0, "z": 0}
                except: pass

                for client in clients:
                        # Receive data:
                        try:
                                client["conn"].recv(MAX_PACKET_SIZE)
                        except: pass

                        # Sync/broadcast clients's data to all clients:
                        sync_packet_data = bytes()
                        for k, v in clients:
                                sync_packet_data += bytes([
                                        k,
                                        v["pos"]["x"],
                                        v["pos"]["y"],
                                        v["pos"]["z"]
                                ])
                        client["conn"].sendall(sync_packet_data)

if is_server:
        server_thread = threading.Thread(target=server, args=(port,))
        server_thread.daemon = True
        server_thread.start()


# Client
peers: dict [Address, Any] = {}

sock = socket.socket(socket.AF_INET6, socket.SOCK_DGRAM)
sock.setblocking(False)
sock.connect((ip, port))
while True:
        pass # TODO: Record, compress, send, receive, decompress, play

