#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <asio.hpp>
#include <memory>
#include <string>

#include "server_parameters.h"

namespace server {

class HTTPServer {
public:
  HTTPServer(asio::io_context &io_context,
             const http_server_parameters_t &http_server_parameters);

private:
};
} // namespace server
#endif // UDP_SERVER_H