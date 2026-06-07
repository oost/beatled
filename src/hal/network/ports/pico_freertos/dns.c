#include <lwip/dns.h>
#include <lwip/pbuf.h>
#include <lwip/tcpip.h>
#include <pico/cyw43_arch.h>
#include <pico/unique_id.h>

#include "hal/network.h"
#include "hal/udp.h"
#include "pico_udp.h"

static bool server_address_resolved = false;
ip_addr_t server_address;
uint16_t server_port;

static void server_dns_found(const char *hostname, const ip_addr_t *ipaddr,
                             void *arg) {

  if (ipaddr != NULL) {
    printf("[NET] DNS resolved %s -> %s\n", hostname, ipaddr_ntoa(ipaddr));
    server_address_resolved = true;
    ip_addr_copy(server_address, *ipaddr);
  } else {
    printf("[ERR] DNS lookup failed for %s\n", hostname);
  }
}

void resolve_server_address(const char *remote_server_name,
                            uint16_t remote_server_port) {
  err_t err;
  server_port = remote_server_port;

  LOCK_TCPIP_CORE();
  err = dns_gethostbyname(remote_server_name, &server_address, server_dns_found,
                          NULL);
  UNLOCK_TCPIP_CORE();

  if (err == ERR_INPROGRESS) {
    return;
  } else if (err == ERR_OK) {
    server_address_resolved = true;
  }
}

void resolve_server_address_blocking(const char *remote_server_name,
                                     uint16_t remote_server_port) {
  resolve_server_address(remote_server_name, remote_server_port);
  uint32_t attempts = 0;
  const uint32_t max_attempts = 1500; // 30 seconds at 20ms intervals
  while (!server_address_resolved && attempts < max_attempts) {
    sleep_ms(20);
    attempts++;
  }
  if (!server_address_resolved) {
    printf("[ERR] DNS resolution timeout for %s\n", remote_server_name);
  } else {
    puts("[NET] Server address resolved");
  }
}

const uint32_t get_ip_address() {
  // w0 on Pico W is the only network interface.
  // Network interfaces start with index 1
  struct netif *netif_ptr = netif_get_by_index(1);
  if (netif_ptr == NULL) {
    printf("[ERR] Network interface #%d not found\n", 1);
    return 0;
  }

  printf("[NET] Interface #%d: %s\n", 1, netif_ptr->name);

  const ip4_addr_t *addr = netif_ip4_addr(netif_ptr);

  return ip_addr_get_ip4_u32(addr);
}

void udp_print_all_ip_addresses() {
  struct netif *netif_ptr;
  int idx = 1;
  char buf[IP4ADDR_STRLEN_MAX];
  while (1) {
    netif_ptr = netif_get_by_index(idx);
    if (netif_ptr == NULL) {
      return;
    }

    printf("[NET] Interface #%d: %s\n", idx, netif_ptr->name);

    const ip4_addr_t *netif_address = netif_ip4_addr(netif_ptr);

    if (ip4addr_ntoa_r(netif_address, buf, IP4ADDR_STRLEN_MAX) == NULL) {
      puts("[ERR] Failed to convert IP address");
      return;
    }
    buf[15] = '\0';
    printf("[NET] Interface #%d address: %s\n", idx, buf);
    idx++;
  }
}
