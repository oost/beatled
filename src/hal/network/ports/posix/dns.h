#ifndef HAL__NETWORK__DNS_H
#define HAL__NETWORK__DNS_H

int resolve_server_address(const char *server_name,
                           struct sockaddr_in *server_addr);

int resolve_server_address_blocking(const char *server_name,
                                    struct sockaddr_in *server_addr);
#endif // HAL__NETWORK__DNS_H