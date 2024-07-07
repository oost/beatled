#ifndef TCP_SERVER__TCP_SERVER_H
#define TCP_SERVER__TCP_SERVER_H

#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>

namespace beatled::server {
using asio::ip::tcp;

class TCPServer {
public:
  TCPServer(asio::io_context &io_context, short port);

private:
  void do_accept();

  tcp::acceptor acceptor_;
  tcp::socket socket_;
};

} // namespace beatled::server

#endif // TCP_SERVER__TCP_SERVER_H
