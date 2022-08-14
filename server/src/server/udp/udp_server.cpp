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

  socket_.async_receive_from(
      asio::buffer(data_, max_length), remote_endpoint_,
      [this](std::error_code ec, std::size_t bytes_recvd) {
        if (!ec && bytes_recvd > 0) {
          std::cout << "Received: " << data_ << std::endl;
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
