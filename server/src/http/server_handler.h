#ifndef SERVER_HANDLER_H
#define SERVER_HANDLER_H

#include <string>

void start_http_server(std::size_t pool_size, const std::string &address, std::uint16_t port, const std::string &root_dir);

#endif


