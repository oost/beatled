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
#include "udp_server/udp_server.hpp"

using namespace beatled::server;
using asio::ip::udp;

UDPServer::UDPServer(const std::string &id, asio::io_context &io_context,
                     const parameters_t &server_parameters,
                     StateManager &state_manager)
    : ServiceControllerInterface{id},
      socket_{io_context, udp::endpoint(udp::v4(), server_parameters.port)},
      state_manager_{state_manager} {
  SPDLOG_INFO("Creating {}", name());
}

void UDPServer::start_sync() {
  SPDLOG_INFO("{}: listening on {}", name(),
              fmt::streamed(socket_.local_endpoint()));
  do_receive();
}
void UDPServer::stop_sync() { socket_.cancel(); }

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

          UDPRequestHandler requestHandler{request_buffer_ptr.get(),
                                           state_manager_};

          DataBuffer::Ptr response_buffer_ptr = requestHandler.response();

          SPDLOG_INFO("Sending response: {::x} to {}", *response_buffer_ptr,
                      fmt::streamed(request_buffer_ptr->remote_endpoint()));

          socket_.async_send_to(
              asio::buffer(response_buffer_ptr->data(),
                           response_buffer_ptr->size()),
              request_buffer_ptr->remote_endpoint(),
              [this](std::error_code /*ec*/, std::size_t /*bytes_sent*/) {});
        }

        if (ec) {
          std::cerr << ec.message() << std::endl;
        }

        do_receive();
      });
}
