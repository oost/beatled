#include <asio/signal_set.hpp>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>
#include <cstdlib>
#include <iostream>
#include <string>

#include "server_parameters.hpp"
#include "udp_server.hpp"

#include "udp_request_handler.hpp"

using namespace server;
using asio::ip::udp;

UDPServer::UDPServer(asio::io_context &io_context,
                     const udp_server_parameters_t &server_parameters,
                     StateManager &state_manager)
    : socket_{io_context, udp::endpoint(udp::v4(), server_parameters.port)},
      state_manager_{state_manager} {
  std::cout << "Starting UDP server, listening on: " << socket_.local_endpoint()
            << std::endl;

  do_receive();
}

void UDPServer::do_receive() {

  std::string recvBuf(max_length_, 0);

  socket_.async_receive_from(
      asio::buffer(recvBuf, max_length_), remote_endpoint_,
      [this, request_body = std::move(recvBuf)](std::error_code ec,
                                                std::size_t bytes_recvd) {
        if (!ec && bytes_recvd > 0) {
          std::string_view request_body_view(request_body);

          UDPRequestHandler requestHandler{
              request_body_view.substr(0, bytes_recvd), remote_endpoint_};
          std::string response = requestHandler.get_response();
          std::cout << "Sending: " << response << " to " << remote_endpoint_
                    << std::endl;
          socket_.async_send_to(
              asio::buffer(response), remote_endpoint_,
              [this](std::error_code /*ec*/, std::size_t /*bytes_sent*/) {});
        }

        if (ec) {
          std::cerr << ec.message() << std::endl;
        }

        do_receive();
      });
}

// void UDPServer::send_response(const std::string &sendBuf,
//                               const asio::ip::udp::endpoint &remote_endpoint)
//                               {
//   // TODO : Is this correct? Does the string need to be moved?
//   std::cout << "Sending: " << sendBuf << " to " << remote_endpoint <<
//   std::endl;

//   socket_.async_send_to(
//       asio::buffer(sendBuf), remote_endpoint,
//       [this](std::error_code /*ec*/, std::size_t /*bytes_sent*/) {});
// }
