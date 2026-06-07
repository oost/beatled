#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "config/constants.h"

#include "hal/time.h"
#include "hal/udp.h"
#include "udp_socket.h"

#define MAXLINE 1024

static pthread_t udp_thread_;
static volatile int udp_running_ = 0;

static void *udp_socket_listen(void *data) {
  udp_parameters_t *params = (udp_parameters_t *)data;

  char buffer[MAXLINE];
  int recvlen;

  struct sockaddr_in remaddr;
  socklen_t addrlen = sizeof(remaddr);

  printf("[NET] Listening on port %d\n", params->udp_port);
  while (udp_running_) {
    recvlen = recvfrom(udp_socket_fd, buffer, MAXLINE - 1, 0,
                       (struct sockaddr *)&remaddr, &addrlen);
    if (recvlen < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        continue;
      }
      if (!udp_running_)
        break;
      printf("[NET] UDP recv error: %s\n", strerror(errno));
      continue;
    }
    if (recvlen > 0) {
      // Stamp arrival before any allocation/copy so dest_time reflects
      // wire arrival, not enqueue time.
      uint64_t rx_time_us = time_us_64();
      buffer[recvlen] = 0;
      void *server_msg = (void *)malloc(recvlen);
      if (!server_msg) {
        puts("[ERR] Failed to allocate UDP message buffer");
        continue;
      }
      memcpy(server_msg, buffer, recvlen);
      if ((params->process_response)(server_msg, recvlen, rx_time_us)) {
        BEATLED_FATAL("Failed to queue UDP message on event loop");
      }
    }
  }

  return NULL;
}

void start_udp(const char *server_name, uint16_t server_port, uint16_t udp_port,
               process_response_fn process_response) {
  udp_parameters_t *params = malloc(sizeof(udp_parameters_t));
  if (!params) {
    perror("Failed to allocate UDP parameters");
    return;
  }
  params->udp_port = udp_port;
  params->process_response = process_response;
  params->server_name = server_name;
  params->server_port = server_port;

  if (create_udp_socket(params)) {
    perror("Error creating sockets");
    return;
  }

  udp_running_ = 1;
  pthread_create(&udp_thread_, NULL, &udp_socket_listen, params);
}

void shutdown_udp_socket() {
  udp_running_ = 0;
  if (udp_socket_fd > 0) {
    close(udp_socket_fd);
    udp_socket_fd = -1;
  }
  pthread_join(udp_thread_, NULL);
}
