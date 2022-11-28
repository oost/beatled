#include <asio.hpp>
#include <fmt/ostream.h>
#include <iostream>
#include <spdlog/spdlog.h>

#include "state_manager/state_manager.hpp"
#include "tempo_broadcaster/broadcast_loop.hpp"
#include "tempo_broadcaster/tempo_broadcaster.hpp"
#include "udp/udp_buffer.hpp"

using asio::ip::udp;

using namespace server;

TempoBroadcaster::TempoBroadcaster(
    asio::io_context &io_context, std::chrono::nanoseconds alarm_period,
    std::chrono::nanoseconds program_alarm_period,
    const broadcasting_server_parameters_t &broadcasting_server_parameters,
    StateManager &state_manager)
    : io_context_(io_context), alarm_period_(alarm_period),
      program_alarm_period_(program_alarm_period), count_(0),
      program_timer_(
          asio::high_resolution_timer(io_context_, program_alarm_period_)),
      timer_(asio::high_resolution_timer(io_context_, alarm_period)),
      socket_(std::make_shared<asio::ip::udp::socket>(io_context)),
      broadcast_address_(
          asio::ip::make_address_v4(broadcasting_server_parameters.address)),
      port_(broadcasting_server_parameters.port),
      program_idx_(0), state_manager_{state_manager} {

  SPDLOG_INFO("Starting TempoBroadcaster ");

  socket_->open(udp::v4());
  if (!socket_->is_open()) {
    throw std::runtime_error("Can't open UDP socket");
  }

  socket_->set_option(udp::socket::reuse_address(true));
  socket_->set_option(asio::socket_base::broadcast(true));

  // socket_->bind(udp::endpoint(broadcast_address_, port_));
  SPDLOG_INFO("TempoBroadcaster broadcasting on UDP through {} {}",
              fmt::streamed(socket_->local_endpoint()),
              fmt::streamed(broadcast_address_));

  loops_.push_back(std::make_unique<BroadcastLoop>(
      socket_, alarm_period,
      [this]() {
        tempo_ref_t tr = state_manager_.get_tempo_ref();

        ResponseBuffer::Ptr response_buffer =
            std::make_unique<TempoResponseBuffer>(tr.beat_time_ref,
                                                  tr.tempo_period_us);
        return std::move(response_buffer);
      },
      broadcasting_server_parameters));

  // loops_.push_back(std::make_unique<BroadcastLoop>(
  //     socket_, program_alarm_period,
  //     [this]() {
  //       std::string sendBuf{"ca"};
  //       // sendBuf[1] = char(program_idx_++ % 7);
  //       // std::cout << int(sendBuf[1]) << std::endl;
  //       return std::move(sendBuf);
  //     },
  //     broadcasting_server_parameters));
}
