#ifndef SERVER_ARGUMENTS_H
#define SERVER_ARGUMENTS_H

#include "http/server_parameters.hpp"
#include "logger/logger_parameters.hpp"
#include "tempo_broadcaster/server_parameters.hpp"
#include "udp/server_parameters.hpp"

namespace server {

struct server_parameters_t {
  bool start_http_server;
  bool start_udp_server;
  bool start_broadcaster;
  http_server_parameters_t http;
  udp_server_parameters_t udp;
  broadcasting_server_parameters_t broadcasting;
  logger_parameters_t logger;
  std::size_t thread_pool_size;
};

} // namespace server

#endif // SERVER_ARGUMENTS_H