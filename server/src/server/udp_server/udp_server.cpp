#include <array>
#include <asio/signal_set.hpp>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>
#include <cstdlib>
#include <iostream>
#include <string>

#include "server_parameters.hpp"
#include "udp/udp_buffer.hpp"
#include "udp_request_handler.hpp"
#include "udp_server.hpp"

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
  std::unique_ptr<UDPRequestBuffer> request_buffer_ptr =
      std::make_unique<UDPRequestBuffer>();

  socket_.async_receive_from(
      asio::buffer(request_buffer_ptr->data(), request_buffer_ptr->BUFFER_SIZE),
      request_buffer_ptr->remote_endpoint(),
      [this, request_buffer_ptr = std::move(request_buffer_ptr)](
          std::error_code ec, std::size_t bytes_recvd) mutable {
        // TODO: Why do we need mutable here?

        if (!ec && bytes_recvd > 0) {

          std::cout << "Received request from  response: "
                    << request_buffer_ptr->remote_endpoint() << std::endl;

          request_buffer_ptr->setSize(bytes_recvd);

          UDPRequestHandler requestHandler{std::move(request_buffer_ptr),
                                           state_manager_};

          UDPResponseBuffer::Ptr response_buffer_ptr =
              requestHandler.response();

          std::cout << "Sending response: " << *response_buffer_ptr << " to "
                    << response_buffer_ptr->remote_endpoint() << std::endl;

          socket_.async_send_to(
              asio::buffer(response_buffer_ptr->data(),
                           response_buffer_ptr->size()),
              response_buffer_ptr->remote_endpoint(),
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
