#ifndef HAL__NETWORK__PICO_UDP_H
#define HAL__NETWORK__PICO_UDP_H

#include <lwip/dns.h>
#include <lwip/pbuf.h>
#include <pico/cyw43_arch.h>

extern struct udp_pcb *server_udp_pcb;

extern ip_addr_t server_address;
extern uint16_t server_port;

void resolve_server_address(const char *server_name,
                            uint16_t remote_server_port);
void resolve_server_address_blocking(const char *server_name,
                                     uint16_t remote_server_port);

#endif // HAL__NETWORK__PICO_UDP_H