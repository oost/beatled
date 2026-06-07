#ifndef HAL__NETWORK__UDP_SOCKET_H
#define HAL__NETWORK__UDP_SOCKET_H

#include <netinet/in.h>
#include <stddef.h>
#include <stdint.h>

#include "hal/udp.h"

typedef struct pbuf {
  void *payload;
} pbuf;

typedef struct udp_parameters {
  const char *server_name;
  uint16_t server_port;
  uint16_t udp_port;
  process_response_fn process_response;
} udp_parameters_t;

extern int udp_socket_fd;
extern struct sockaddr_in server_addr;

int create_udp_socket(udp_parameters_t *udp_params);
int sendall(int socket_fd, char *data_buffer, size_t *data_length,
            const struct sockaddr_in *recipient_addr);
uint32_t get_ip_address();
void udp_print_all_ip_addresses();
int send_udp_request(size_t msg_length, prepare_payload_fn prepare_payload);

#endif // HAL__NETWORK__UDP_SOCKET_H
