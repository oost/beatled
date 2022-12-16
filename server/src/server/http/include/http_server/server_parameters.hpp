#ifndef SERVER_HTTP_PARAMETERS
#define SERVER_HTTP_PARAMETERS

// #include <cstdint>
#include <string>

namespace server {

struct http_server_parameters_t {
  const std::string address;
  std::uint16_t port;
  const std::string root_dir;
  const std::string certs_dir;
};

} // namespace server

#endif // SERVER_HTTP_PARAMETERS