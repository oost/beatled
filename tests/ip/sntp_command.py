import socket, struct, sys, time

NTP_SERVER = "localhost"
TIME1970 = 2208988800


def send_command():
    client = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    data = "T1"
    print("Sending data")
    client.sendto(data.encode("utf-8"), (NTP_SERVER, 9090))
    print("Data sent")
    data, address = client.recvfrom(1024)
    if data:
        print("Response received from:", address)
    # t = struct.unpack("!12I", data)[10] - TIME1970
    # print("\tTime = %s" % time.ctime(t))
    print(data)


if __name__ == "__main__":
    send_command()
