#ifndef TCP_SERVER__TCP_SESSION_H
#define TCP_SERVER__TCP_SESSION_H

#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>

namespace server {

using asio::ip::tcp;

class TCPSession : public std::enable_shared_from_this<TCPSession> {
public:
  TCPSession(tcp::socket socket);

  void start();

private:
  void do_read();

  void do_write(std::size_t length);

  tcp::socket socket_;
  enum { max_length = 1024 };
  char data_[max_length];
};

} // namespace server

#endif // TCP_SERVER__TCP_SERVER_H
