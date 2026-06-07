#ifndef HAL__UDP_H
#define HAL__UDP_H

#ifdef __cplusplus
extern "C" {
#endif
// #include <lwip/pbuf.h>
#include <stdlib.h>

#include <stdint.h>

typedef int (*prepare_payload_fn)(void *buffer_payload, size_t size);

// rx_time_us is the local monotonic timestamp captured by the network port
// at packet arrival (top of dgram_recv / equivalent). Callers should pass it
// straight through to the event queue so downstream consumers — chiefly the
// time-sync algorithm — see arrival-time, not dequeue-time.
typedef int (*process_response_fn)(void *buffer_payload, size_t size,
                                   uint64_t rx_time_us);

// void resolve_server_address(const char *server_name);
// void resolve_server_address_blocking(const char *server_name);

void start_udp(const char *server_name, uint16_t server_port, uint16_t udp_port,
               process_response_fn process_response);
void shutdown_udp_socket();

// Perform initialisation

uint32_t get_ip_address();
void udp_print_all_ip_addresses();

int send_udp_request(size_t msg_length, prepare_payload_fn prepare_payload);

#ifdef __cplusplus
}
#endif

#endif // HAL__UDP_H