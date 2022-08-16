#include <asio/signal_set.hpp>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>
#include <cstdlib>
#include <iostream>
#include <string>

#include "server_parameters.hpp"
#include "udp_server.hpp"

using namespace server;
using asio::ip::udp;

UDPServer::UDPServer(asio::io_context &io_context,
                     const udp_server_parameters_t &server_parameters)
    : socket_(io_context, udp::endpoint(udp::v4(), server_parameters.port)) {
  std::cout << "Starting UDP server, listening on: " << socket_.local_endpoint()
            << std::endl;

  do_receive();
}

void UDPServer::do_receive() {
  std::cout << "do_receive" << std::endl;

  char *recv_buf = new char[max_length];
  socket_.async_receive_from(
      asio::buffer(recv_buf, max_length), remote_endpoint_,
      [this, recv_buf](std::error_code ec, std::size_t bytes_recvd) {
        if (!ec && bytes_recvd > 0) {
          std::cout << "Received: " << recv_buf << std::endl;
          do_send(bytes_recvd, recv_buf);
        } else {
          do_receive();
        }
      });
}

void UDPServer::do_send(std::size_t length, char *recv_buf) {
  socket_.async_send_to(
      asio::buffer(recv_buf, length), remote_endpoint_,
      [this, recv_buf](std::error_code /*ec*/, std::size_t /*bytes_sent*/) {
        delete[] recv_buf, do_receive();
      });
}
