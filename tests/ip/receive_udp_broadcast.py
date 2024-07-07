import socket

# UDP_IP = "192.168.86.20"
# UDP_PORT = 8765
UDP_IP = ""

# UDP_IP = "localhost"
# UDP_PORT = 123
UDP_PORT = 9090

print("UDP target IP: %s" % UDP_IP)
print("UDP target port: %s" % UDP_PORT)

# interfaces = socket.getaddrinfo(
#     host=socket.gethostname(), port=None, family=socket.AF_INET
# )
# allips = [ip[-1][0] for ip in interfaces]
# print(allips)


sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)  # Internet  # UDP
sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

sock.bind((UDP_IP, UDP_PORT))
while True:
    data, addr = sock.recvfrom(1024)  # buffer size is 1024 bytes
    print(f"received message: {data.hex()}, size {len(data)} from {addr}")
