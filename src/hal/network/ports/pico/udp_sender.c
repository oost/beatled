#include <lwip/dns.h>
#include <lwip/pbuf.h>
#include <pico/cyw43_arch.h>
#include <pico/unique_id.h>

#include "hal/network.h"
#include "hal/udp.h"
#include "pico_udp.h"

void *udp_buffer_alloc(size_t msg_length) {

  struct pbuf *msg_pbuf = pbuf_alloc(PBUF_TRANSPORT, msg_length, PBUF_RAM);

  if (!msg_pbuf) {
    puts("[ERR] Failed to allocate pbuf");
    return NULL;
  }
  return msg_pbuf->payload;
}

void print_buffer_as_hex(uint8_t *buffer, int buffer_length) {
  int i;
  for (i = 0; i < buffer_length; i++) {
    if (i > 0)
      printf(":");
    printf("%02X", buffer[i]);
  }
}

int send_udp_request(size_t msg_length, prepare_payload_fn prepare_payload) {
  int err = 0;
  cyw43_arch_lwip_begin();

  struct pbuf *buffer = pbuf_alloc(PBUF_TRANSPORT, msg_length, PBUF_RAM);
  if (!buffer) {
    puts("[ERR] Failed to allocate pbuf");
    err = 1;
  } else {

    uint8_t *req = (uint8_t *)buffer->payload;
    memset(req, 0, msg_length);

    if (prepare_payload(buffer->payload, msg_length) != 0) {
      puts("[ERR] Failed to prepare UDP payload");
      err = 1;
    }

    err_t send_err =
        udp_sendto(server_udp_pcb, buffer, &server_address, server_port);
    if (send_err != ERR_OK) {
      printf("[ERR] udp_sendto -> %s:%u len=%zu err=%d\n",
             ipaddr_ntoa(&server_address), server_port, msg_length, send_err);
      err = 1;
    } else {
      printf("[NET] udp_sendto -> %s:%u len=%zu OK\n",
             ipaddr_ntoa(&server_address), server_port, msg_length);
    }
    pbuf_free(buffer);
  }
  cyw43_arch_lwip_end();

  return err;
}
