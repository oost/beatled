import socket
import time

UDP_IP = "192.168.86.42"
# UDP_IP = "raspberrypiz.local"
# UDP_PORT = 9090
UDP_PORT = 9090
# MESSAGE = b"Hello, World Broadcast!!!hh"

print("UDP target IP: %s" % UDP_IP)
print("UDP target port: %s" % UDP_PORT)
# print("message: %s" % MESSAGE)

# interfaces = socket.getaddrinfo(
#     host=socket.gethostname(), port=None, family=socket.AF_INET
# )
# allips = [ip[-1][0] for ip in interfaces]
# print(allips)

rList = [1,1]

MESSAGE = bytes(rList)

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)  # Internet  # UDP
sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
sock.bind(("0.0.0.0", 0))

addrs = socket.getaddrinfo("localhost", None)
addr = addrs[0][4][0]

while 1:
    for pattern_idx in range(7):
        rList = [1, pattern_idx]
        print(f"Sending {rList} to {addr}:{UDP_PORT}")
        MESSAGE = bytes(rList)
        sock.sendto(MESSAGE, (addr, UDP_PORT))
        time.sleep(3)
