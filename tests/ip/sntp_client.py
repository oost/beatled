import socket, struct, sys, time

NTP_SERVER = "raspberrypi1.local"
TIME1970 = 2208988800


def sntp_client():
    client = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    data = "\x1b" + 47 * "\0"
    print("Sending data")
    client.sendto(data.encode("utf-8"), (NTP_SERVER, 123))
    print("Data sent")
    data, address = client.recvfrom(1024)
    if data:
        print("Response received from:", address)
    t = struct.unpack("!12I", data)[10] - TIME1970
    print("\tTime = %s" % time.ctime(t))


if __name__ == "__main__":
    sntp_client()
