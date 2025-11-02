import sys
import ipaddress
import socket
import pyaudio
import pyogg
import openal


MAX_CLIENTS = 8
MAX_PACKET_SIZE = 65536


class PacketType:
        SET_NAME = 0
        VOICE_DATA = 1
        POSITION_DATA = 2


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

ip = None
if len(sys.argv) == 3:
        try:
                ipaddress.IPv6Address(sys.argv[2])
        except Exception as e:
                print(f"ERROR: Provided IPv6 address {sys.argv[2]} is invalid")
                print(e)
                exit(1)
        ip = sys.argv[2]


sock = socket.socket(socket.AF_INET6, socket.SOCK_DGRAM)
sock.settimeout(0)

if ip:
        sock.connect((ip, port))
        while True:
                pass # TODO: Record, compress, send, receive, decompress, play

else:
        clients: dict[socket.socket, str] = []
        sock.bind(("", port))
        while True:
                sock.listen(MAX_CLIENTS)
                clients.append(sock.accept()[0])
                for i in range(1, len(clients)+1):
                        pass # TODO

sock.close()
