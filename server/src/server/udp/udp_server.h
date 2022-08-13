#ifndef UDP_SERVER_H
#define UDP_SERVER_H

#include <asio.hpp>
#include <memory>
#include <string>

#include "server_parameters.h"

namespace server {

class UDPServer {
public:
  UDPServer(asio::io_context &io_context,
            const udp_server_parameters_t &server_parameters);

private:
  void do_receive();

  void do_send(size_t length);

  asio::ip::udp::socket socket_;
  asio::ip::udp::endpoint remote_endpoint_;
  enum { max_length = 1024 };
  char data_[max_length];
};
} // namespace server
#endif // UDP_SERVER_H