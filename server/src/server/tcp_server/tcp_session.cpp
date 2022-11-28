#include "tcp_server/tcp_session.hpp"

using namespace server;

TCPSession::TCPSession(tcp::socket socket) : socket_(std::move(socket)) {}

void TCPSession::start() { do_read(); }

void TCPSession::do_read() {
  auto self(shared_from_this());
  socket_.async_read_some(asio::buffer(data_, max_length),
                          [this, self](std::error_code ec, std::size_t length) {
                            if (!ec) {
                              do_write(length);
                            }
                          });
}

void TCPSession::do_write(std::size_t length) {
  auto self(shared_from_this());
  asio::async_write(socket_, asio::buffer(data_, length),
                    [this, self](std::error_code ec, std::size_t /*length*/) {
                      if (!ec) {
                        do_read();
                      }
                    });
}
