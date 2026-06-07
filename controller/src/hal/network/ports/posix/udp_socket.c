#include <arpa/inet.h>
#include <errno.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "config/constants.h"

#include "dns.h"
#include "udp_socket.h"

int udp_socket_fd;
struct sockaddr_in server_addr;

int create_udp_socket(udp_parameters_t *udp_params) {
  struct sockaddr_in *addr;
  struct sockaddr_in device_addr;
  if (resolve_server_address_blocking(udp_params->server_name, &server_addr)) {
    perror("socket creation failed");
    return 1;
  }
  addr = (struct sockaddr_in *)&server_addr;
  addr->sin_port = htons(udp_params->server_port);

  if ((udp_socket_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
    perror("socket creation failed");
    return 1;
  }

  // Filling server information
  device_addr.sin_family = AF_INET; // IPv4
  device_addr.sin_addr.s_addr = INADDR_ANY;
  device_addr.sin_port = htons(udp_params->udp_port);

  // Set receive timeout (30 seconds)
  struct timeval tv = {.tv_sec = 30, .tv_usec = 0};
  if (setsockopt(udp_socket_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) <
      0) {
    perror("setsockopt SO_RCVTIMEO failed");
  }

  // Bind the socket with the server address
  if (bind(udp_socket_fd, (const struct sockaddr *)&device_addr,
           sizeof(device_addr)) < 0) {
    perror("bind failed");
    return 1;
  }
  return 0;
}

int sendall(int socket_fd, char *data_buffer, size_t *data_length,
            const struct sockaddr_in *recipient_addr) {
  int total = 0;                // how many bytes we've sent
  int bytesleft = *data_length; // how many we have left to send
  int n;
  int retries = 0;
  const int max_retries = 3;

  while (total < *data_length) {
    n = sendto(socket_fd, data_buffer + total, bytesleft, 0,
               (const struct sockaddr *)recipient_addr,
               sizeof(*recipient_addr));
    if (n == -1) {
      if ((errno == EAGAIN || errno == EWOULDBLOCK) &&
          retries < max_retries) {
        retries++;
        printf("[NET] UDP send retry (%d/%d)\n", retries, max_retries);
        usleep(retries * 1000); // 1-3ms backoff
        continue;
      }
      printf("[ERR] UDP send failed: %s\n", strerror(errno));
      break;
    }
    retries = 0;
    total += n;
    bytesleft -= n;
  }

  *data_length = total; // return number actually sent here

  return n == -1 ? -1 : 0; // return -1 on failure, 0 on success
}

uint32_t get_ip_address() {
  unsigned char ip_address[15];
  int fd;
  struct ifreq ifr;

  /*AF_INET - to define network interface IPv4*/
  /*Creating soket for it.*/
  fd = socket(AF_INET, SOCK_DGRAM, 0);

  /*AF_INET - to define IPv4 Address type.*/
  ifr.ifr_addr.sa_family = AF_INET;

  /*eth0 - define the ifr_name - port name
  where network attached.*/
  memcpy(ifr.ifr_name, "eth0", IFNAMSIZ - 1);

  /*Accessing network interface information by
  passing address using ioctl.*/
  ioctl(fd, SIOCGIFADDR, &ifr);
  /*closing fd*/
  close(fd);

  /*Extract IP Address*/
  strcpy((void *)ip_address,
         inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));

  printf("[NET] System IP: %s\n", ip_address);
  return 0;
}

void udp_print_all_ip_addresses() {
  struct ifaddrs *ifaddr;
  int family, s;
  char host[NI_MAXHOST];

  if (getifaddrs(&ifaddr) == -1) {
    perror("getifaddrs");
    exit(EXIT_FAILURE);
  }

  /* Walk through linked list, maintaining head pointer so we
     can free list later. */

  for (struct ifaddrs *ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
    if (ifa->ifa_addr == NULL)
      continue;

    family = ifa->ifa_addr->sa_family;

    /* Display interface name and family (including symbolic
       form of the latter for the common families). */

    printf("[NET] %-8s %s (%d)\n", ifa->ifa_name,
           (family == AF_INET)    ? "AF_INET"
           : (family == AF_INET6) ? "AF_INET6"
                                  : "???",
           family);

    /* For an AF_INET* interface address, display the address. */

    if (family == AF_INET || family == AF_INET6) {
      s = getnameinfo(ifa->ifa_addr,
                      (family == AF_INET) ? sizeof(struct sockaddr_in)
                                          : sizeof(struct sockaddr_in6),
                      host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
      if (s != 0) {
        printf("[ERR] getnameinfo() failed: %s\n", gai_strerror(s));
        exit(EXIT_FAILURE);
      }

      printf("[NET]   address: %s\n", host);
    }
  }

  freeifaddrs(ifaddr);
}

int send_udp_request(size_t msg_length, prepare_payload_fn prepare_payload) {
  int err = 0;
  pbuf *buffer = (pbuf *)malloc(sizeof(pbuf));
  if (!buffer) {
    puts("[ERR] Failed to allocate UDP send buffer");
    return 1;
  }

  buffer->payload = malloc(msg_length);
  if (!buffer->payload) {
    puts("[ERR] Failed to allocate UDP payload buffer");
    err = 1;
  } else {

    uint8_t *req = (uint8_t *)buffer->payload;
    memset(req, 0, msg_length);

    if (prepare_payload(buffer->payload, msg_length) != 0) {
      puts("[ERR] Failed to prepare UDP payload");
      err = 1;
    }

    if (sendall(udp_socket_fd, buffer->payload, &msg_length, &server_addr)) {
      puts("[ERR] Failed to send UDP message");
    }
  }

  char ip4[INET_ADDRSTRLEN]; // space to hold the IPv4 string
  const struct sockaddr_in *recipient_addr =
      (const struct sockaddr_in *)&server_addr;
  inet_ntop(AF_INET, &(recipient_addr->sin_addr), ip4, INET_ADDRSTRLEN);
#if BEATLED_VERBOSE_LOG
  printf("[NET] Sent UDP request to %s:%d\n", ip4, ntohs(recipient_addr->sin_port));
#endif

  free(buffer->payload);
  free(buffer);

  return err;
}
