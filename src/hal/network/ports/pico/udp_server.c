#include <lwip/dns.h>
#include <lwip/pbuf.h>
#include <pico/cyw43_arch.h>
#include <pico/time.h>
#include <pico/unique_id.h>

#include "hal/network.h"
#include "hal/udp.h"
#include "pico_udp.h"

struct udp_pcb *server_udp_pcb;
static process_response_fn process_response_;

static uint32_t msg_id = 0;

void dgram_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p,
                const ip_addr_t *addr, u16_t port) {
  // Stamp the arrival time first — anything else here (malloc, copy,
  // queue contention) leaks into the time-sync algorithm if measured later.
  uint64_t rx_time_us = time_us_64();

  size_t data_length = p->tot_len;
  void *server_msg = (void *)malloc(data_length);
  if (!server_msg) {
    printf("[ERR] Failed to allocate %zu bytes for UDP message\n", data_length);
    pbuf_free(p);
    return;
  }

  uint16_t copied_length = pbuf_copy_partial(p, server_msg, data_length, 0);
  pbuf_free(p);

  int server_msg_enqueue_error = 1;

  if (copied_length > 0 && copied_length == data_length) {
    server_msg_enqueue_error = process_response_(server_msg, data_length, rx_time_us);
    if (server_msg_enqueue_error) {
      puts("[ERR] Failed to queue UDP message on event loop");
    }
  } else {
    puts("[ERR] UDP message copy failed");
  }

  if (server_msg_enqueue_error) {
    printf("[ERR] Failed to enqueue UDP message, freeing buffer\n");
    free(server_msg);
  }
}

// Perform initialisation
int init_udp_socket(uint16_t udp_port) {
  cyw43_arch_lwip_begin();

  server_udp_pcb = udp_new_ip_type(IPADDR_TYPE_ANY);
  if (!server_udp_pcb) {
    puts("[ERR] Failed to create UDP PCB");
    cyw43_arch_lwip_end();

    return 1;
  }

  // Bind to endpoint  IP_ADDR_BROADCAST IP_ANY_TYPE
  if (udp_bind(server_udp_pcb, IP_ANY_TYPE, udp_port)) {
    puts("[ERR] Failed to bind UDP PCB");
    udp_remove(server_udp_pcb);
    cyw43_arch_lwip_end();

    return 1;
  }

  // ip_set_option(server_udp_pcb, SOF_BROADCAST);
  ip_set_option(server_udp_pcb, IP_SOF_BROADCAST_RECV);

  // Add callback
  udp_recv(server_udp_pcb, dgram_recv, NULL);
  printf("[NET] Listening on %s:%d\n",
         ipaddr_ntoa(&server_udp_pcb->local_ip), udp_port);
  cyw43_arch_lwip_end();

  return 0;
}

void shutdown_udp_socket() {
  cyw43_arch_lwip_begin();
  if (server_udp_pcb) {
    udp_remove(server_udp_pcb);
    server_udp_pcb = NULL;
  }
  cyw43_arch_lwip_end();
}

void start_udp(const char *server_name, uint16_t server_port, uint16_t udp_port,
               process_response_fn process_response) {
  process_response_ = process_response;
  resolve_server_address_blocking(server_name, server_port);
  init_udp_socket(udp_port);
}
