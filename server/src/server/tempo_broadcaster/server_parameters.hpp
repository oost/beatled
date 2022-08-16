#ifndef SERVER_BROADCASTING_PARAMETERS
#define SERVER_BROADCASTING_PARAMETERS

// #include <cstdint>
#include <string>

namespace server {

struct broadcasting_server_parameters_t {
  const std::string address;
  std::uint16_t port;
};

} // namespace server

#endif // SERVER_BROADCASTING_PARAMETERS