#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "FreeRTOS.h"
#include "task.h"

#include "config/constants.h"

#include "hal/time.h"
#include "hal/udp.h"
#include "../posix/udp_socket.h"

#define MAXLINE 1024

static TaskHandle_t udp_task_handle = NULL;
static volatile int udp_running_ = 0;

static void udp_socket_listen(void *data) {
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
      uint64_t rx_time_us = time_us_64();
      buffer[recvlen] = 0;
      void *server_msg = (void *)pvPortMalloc(recvlen);
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

  vPortFree(params);
  vTaskDelete(NULL);
}

void start_udp(const char *server_name, uint16_t server_port, uint16_t udp_port,
               process_response_fn process_response) {
  udp_parameters_t *params = pvPortMalloc(sizeof(udp_parameters_t));
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
  xTaskCreate(udp_socket_listen, "UDP", 4096, params, tskIDLE_PRIORITY + 1,
              &udp_task_handle);
}

void shutdown_udp_socket() {
  udp_running_ = 0;
  if (udp_socket_fd > 0) {
    close(udp_socket_fd);
    udp_socket_fd = -1;
  }
  // Task will self-delete when the listen loop exits
}
