#include <lwip/dns.h>
#include <lwip/pbuf.h>
#include <lwip/tcpip.h>
#include <pico/cyw43_arch.h>
#include <pico/unique_id.h>

#include "hal/network.h"
#include "hal/udp.h"
#include "pico_udp.h"

void *udp_buffer_alloc(size_t msg_length) {

  LOCK_TCPIP_CORE();
  struct pbuf *msg_pbuf = pbuf_alloc(PBUF_TRANSPORT, msg_length, PBUF_RAM);
  UNLOCK_TCPIP_CORE();

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

  LOCK_TCPIP_CORE();
  struct pbuf *buffer = pbuf_alloc(PBUF_TRANSPORT, msg_length, PBUF_RAM);
  UNLOCK_TCPIP_CORE();
  if (!buffer) {
    puts("[ERR] Failed to allocate pbuf");
    return 1;
  }

  uint8_t *req = (uint8_t *)buffer->payload;
  memset(req, 0, msg_length);

  if (prepare_payload(buffer->payload, msg_length) != 0) {
    puts("[ERR] Failed to prepare UDP payload");
    err = 1;
  }

  LOCK_TCPIP_CORE();
  err_t send_err =
      udp_sendto(server_udp_pcb, buffer, &server_address, server_port);
  pbuf_free(buffer);
  UNLOCK_TCPIP_CORE();

  printf("[NET] udp_sendto -> %s:%u len=%zu err=%d\n",
         ipaddr_ntoa(&server_address), server_port, msg_length, send_err);

  if (send_err != ERR_OK) {
    err = 1;
  }

  return err;
}
