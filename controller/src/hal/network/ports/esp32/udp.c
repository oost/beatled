#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_netif.h"

#include "config/constants.h"

#include "dns.h"
#include "hal/time.h"
#include "hal/udp.h"

#define MAXLINE 1024

static TaskHandle_t udp_task_handle = NULL;
static volatile int udp_running_ = 0;

static int udp_socket_fd;
static struct sockaddr_in server_addr;

typedef struct udp_buf {
  void *payload;
} udp_buf_t;

typedef struct udp_parameters {
  const char *server_name;
  uint16_t server_port;
  uint16_t udp_port;
  process_response_fn process_response;
} udp_parameters_t;

static int create_udp_socket(udp_parameters_t *udp_params) {
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

  device_addr.sin_family = AF_INET;
  device_addr.sin_addr.s_addr = INADDR_ANY;
  device_addr.sin_port = htons(udp_params->udp_port);

  struct timeval tv = {.tv_sec = 30, .tv_usec = 0};
  if (setsockopt(udp_socket_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) <
      0) {
    perror("setsockopt SO_RCVTIMEO failed");
  }

  if (bind(udp_socket_fd, (const struct sockaddr *)&device_addr,
           sizeof(device_addr)) < 0) {
    perror("bind failed");
    return 1;
  }
  return 0;
}

static int sendall(int socket_fd, char *data_buffer, size_t *data_length,
                   const struct sockaddr_in *recipient_addr) {
  int total = 0;
  int bytesleft = *data_length;
  int n = -1;
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
        vTaskDelay(pdMS_TO_TICKS(retries));
        continue;
      }
      printf("[ERR] UDP send failed: %s\n", strerror(errno));
      break;
    }
    retries = 0;
    total += n;
    bytesleft -= n;
  }

  *data_length = total;
  return n == -1 ? -1 : 0;
}

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

void start_udp(const char *server_name, uint16_t server_port,
               uint16_t udp_port, process_response_fn process_response) {
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
}

uint32_t get_ip_address() {
  esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
  if (netif) {
    esp_netif_ip_info_t ip_info;
    if (esp_netif_get_ip_info(netif, &ip_info) == ESP_OK) {
      printf("[NET] System IP: " IPSTR "\n", IP2STR(&ip_info.ip));
      return ip_info.ip.addr;
    }
  }
  puts("[NET] No IP address available");
  return 0;
}

void udp_print_all_ip_addresses() {
  esp_netif_t *netif = NULL;
  while ((netif = esp_netif_next_unsafe(netif)) != NULL) {
    esp_netif_ip_info_t ip_info;
    if (esp_netif_get_ip_info(netif, &ip_info) == ESP_OK) {
      const char *desc = esp_netif_get_desc(netif);
      printf("[NET] %-8s " IPSTR "\n", desc ? desc : "???",
             IP2STR(&ip_info.ip));
    }
  }
}

int send_udp_request(size_t msg_length, prepare_payload_fn prepare_payload) {
  int err = 0;
  udp_buf_t *buffer = (udp_buf_t *)pvPortMalloc(sizeof(udp_buf_t));
  if (!buffer) {
    puts("[ERR] Failed to allocate UDP send buffer");
    return 1;
  }

  buffer->payload = pvPortMalloc(msg_length);
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

  char ip4[INET_ADDRSTRLEN];
  const struct sockaddr_in *recipient_addr =
      (const struct sockaddr_in *)&server_addr;
  inet_ntop(AF_INET, &(recipient_addr->sin_addr), ip4, INET_ADDRSTRLEN);
#if BEATLED_VERBOSE_LOG
  printf("[NET] Sent UDP request to %s:%d\n", ip4,
         ntohs(recipient_addr->sin_port));
#endif

  vPortFree(buffer->payload);
  vPortFree(buffer);

  return err;
}
