#include <asio/signal_set.hpp>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>
#include <cstdlib>
#include <iostream>
#include <string>

#include "server_parameters.h"
#include "udp_server.h"

using namespace server;
using asio::ip::udp;

enum { max_length = 1024 };

UDPServer::UDPServer(asio::io_context &io_context,
                     const udp_server_parameters_t &server_parameters)
    : socket_(io_context, udp::endpoint(udp::v4(), server_parameters.port)) {
  std::cout << "Starting UDP server, listening on: " << socket_.local_endpoint()
            << std::endl;

  do_receive();
}

void UDPServer::do_receive() {
  socket_.async_receive_from(
      asio::buffer(data_, max_length), remote_endpoint_,
      [this](std::error_code ec, std::size_t bytes_recvd) {
        if (!ec && bytes_recvd > 0) {
          std::cout << "Received: " << data_ << "\n";
          do_send(bytes_recvd);
        } else {
          do_receive();
        }
      });
}

void UDPServer::do_send(std::size_t length) {
  socket_.async_send_to(asio::buffer(data_, length), remote_endpoint_,
                        [this](std::error_code /*ec*/,
                               std::size_t /*bytes_sent*/) { do_receive(); });
}
