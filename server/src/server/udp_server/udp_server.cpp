#include <array>
#include <asio/signal_set.hpp>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>
#include <cstdlib>
#include <fmt/ostream.h>
#include <iostream>
#include <spdlog/spdlog.h>
#include <string>

#include "udp/udp_buffer.hpp"
#include "udp_request_handler.hpp"
#include "udp_server/server_parameters.hpp"
#include "udp_server/udp_server.hpp"

using namespace server;
using asio::ip::udp;

UDPServer::UDPServer(asio::io_context &io_context,
                     const udp_server_parameters_t &server_parameters,
                     StateManager &state_manager)
    : socket_{io_context, udp::endpoint(udp::v4(), server_parameters.port)},
      state_manager_{state_manager} {
  SPDLOG_INFO("Starting UDP server, listening on: {}",
              fmt::streamed(socket_.local_endpoint()));

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

          SPDLOG_INFO("Received request from  response: {}",
                      fmt::streamed(request_buffer_ptr->remote_endpoint()));

          request_buffer_ptr->setSize(bytes_recvd);

          UDPRequestHandler requestHandler{std::move(request_buffer_ptr),
                                           state_manager_};

          UDPResponseBuffer::Ptr response_buffer_ptr =
              requestHandler.response();

          SPDLOG_INFO("Sending response: {::x} to {}", *response_buffer_ptr,
                      fmt::streamed(response_buffer_ptr->remote_endpoint()));

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
//   SPDLOG_INFO("Sending: " << sendBuf << " to " << remote_endpoint <<
//   std::endl;

//   socket_.async_send_to(
//       asio::buffer(sendBuf), remote_endpoint,
//       [this](std::error_code /*ec*/, std::size_t /*bytes_sent*/) {});
// }
