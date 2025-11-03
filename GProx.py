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


# Server (broadcast relay)
def server(port):
        clients: list[socket.socket] = []

        def client_broadcast_handler(conn: socket.socket):
                while True:
                        data = conn.recvfrom(MAX_PACKET_SIZE)
                        if not data:
                                clients.remove(conn)
                                conn.close()
                                break
                        for client in clients:
                                if client != conn:
                                        client.send(data)

        sock = socket.socket(socket.AF_INET6, socket.SOCK_DGRAM)
        sock.bind(("", port))
        sock.listen(MAX_CLIENTS)
        while True:
                conn, _ = sock.accept()
                clients.append(conn)
                client_broadcast_handler_thread = threading.Thread(target=client_broadcast_handler, args=(conn,))
                client_broadcast_handler_thread.daemon = True
                client_broadcast_handler_thread.start()

if is_server:
        server_thread = threading.Thread(target=server, args=(port,))
        server_thread.daemon = True
        server_thread.start()


# Client
def listen(conn: socket.socket):
        print(conn.recvfrom(MAX_PACKET_SIZE).decode()) # TEMP; TEST

sock = socket.socket(socket.AF_INET6, socket.SOCK_DGRAM)
sock.connect((ip, port))
while True:
        pass # TODO

