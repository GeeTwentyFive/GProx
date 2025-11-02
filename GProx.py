import sys
import ipaddress
import socket
import pyaudio
import openal


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

if len(sys.argv) == 3:
        try:
                ipaddress.IPv6Address(sys.argv[2])
        except Exception as e:
                print(f"ERROR: Provided IPv6 address {sys.argv[2]} is invalid")
                print(e)
                exit(1)
ip = sys.argv[2]


# TODO: Start server or connect based on `ip`