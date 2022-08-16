#include <asio.hpp>
#include <iostream>

#include "../../state_manager/state_manager.hpp"
#include "broadcast_loop.hpp"
#include "tempo_broadcaster.hpp"

using asio::ip::udp;

using namespace server;

TempoBroadcaster::TempoBroadcaster(
    asio::io_context &io_context, std::chrono::nanoseconds alarm_period,
    std::chrono::nanoseconds program_alarm_period,
    const broadcasting_server_parameters_t &broadcasting_server_parameters)
    : io_context_(io_context), alarm_period_(alarm_period),
      program_alarm_period_(program_alarm_period), count_(0),
      program_timer_(
          asio::high_resolution_timer(io_context_, program_alarm_period_)),
      timer_(asio::high_resolution_timer(io_context_, alarm_period)),
      socket_(std::make_shared<asio::ip::udp::socket>(io_context)),
      broadcast_address_(
          asio::ip::make_address_v4(broadcasting_server_parameters.address)),
      port_(broadcasting_server_parameters.port) {

  std::cout << "Starting TempoBroadcaster " << std::endl;

  socket_->open(udp::v4());
  if (!socket_->is_open()) {
    throw std::runtime_error("Can't open UDP socket");
  }

  socket_->set_option(udp::socket::reuse_address(true));
  socket_->set_option(asio::socket_base::broadcast(true));

  // socket_->bind(udp::endpoint(broadcast_address_, port_));
  std::cout << "TempoBroadcaster broadcasting on UDP through "
            << socket_->local_endpoint() << " " << broadcast_address_
            << std::endl;

  loops_.push_back(std::make_unique<BroadcastLoop>(
      socket_, alarm_period,
      []() {
        char *sendBuf = new char[1];
        sendBuf[0] = 4;
        return sendBuf;
      },
      [](char *buf) { delete[] buf; }, broadcasting_server_parameters));

  loops_.push_back(std::make_unique<BroadcastLoop>(
      socket_, program_alarm_period,
      []() {
        char *sendBuf = new char[2];
        sendBuf[0] = 2;
        sendBuf[1] = 1;
        return sendBuf;
      },
      [](char *buf) { delete[] buf; }, broadcasting_server_parameters));
}
