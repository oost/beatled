#include "tcp_server/tcp_server.hpp"
#include "tcp_server/tcp_session.hpp"

using namespace server;

TCPServer::TCPServer(asio::io_context &io_context, short port)
    : acceptor_(io_context, tcp::endpoint(tcp::v4(), port)),
      socket_(io_context) {
  do_accept();
}

void TCPServer::do_accept() {
  acceptor_.async_accept(socket_, [this](std::error_code ec) {
    if (!ec) {
      std::make_shared<TCPSession>(std::move(socket_))->start();
    }

    do_accept();
  });
}
