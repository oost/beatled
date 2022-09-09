#ifndef UDP_SERVER_H
#define UDP_SERVER_H

#include <asio.hpp>
#include <memory>
#include <string>

#include "server_parameters.hpp"

namespace server {

class UDPServer {
public:
  UDPServer(asio::io_context &io_context,
            const udp_server_parameters_t &server_parameters);

private:
  void do_receive();

  void do_send(std::size_t length, const std::string &sendBuf);
  asio::ip::udp::socket socket_;
  asio::ip::udp::endpoint remote_endpoint_;
  int max_length_ = 1024;
};
} // namespace server
#endif // UDP_SERVER_H