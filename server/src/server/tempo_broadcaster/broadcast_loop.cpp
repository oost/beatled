#include <asio.hpp>
#include <fmt/ostream.h>
#include <iostream>
#include <spdlog/spdlog.h>

#include "broadcast_loop.hpp"
#include "core/clock.hpp"
#include "core/state_manager.hpp"

namespace beatled::server {

using asio::ip::udp;
using beatled::core::Clock;

BroadcastLoop::BroadcastLoop(
    std::shared_ptr<asio::ip::udp::socket> socket,
    std::chrono::nanoseconds alarm_period,
    std::function<DataBuffer::Ptr(void)> prepare_buffer,
    const TempoBroadcaster::parameters_t &broadcasting_server_parameters)
    : alarm_period_{alarm_period}, count_{0},
      timer_{asio::high_resolution_timer(socket->get_executor(), alarm_period)},
      socket_{socket}, prepare_buffer_{prepare_buffer},
      broadcast_address_{udp::endpoint(
          asio::ip::make_address_v4(broadcasting_server_parameters.address),
          broadcasting_server_parameters.port)} {
  SPDLOG_INFO("Creating Broadcast loop ");

  do_broadcast();
}

BroadcastLoop::~BroadcastLoop() {
  SPDLOG_INFO("Destroying BroadcastLoop");
  std::size_t num_cancelled = timer_.cancel();
  SPDLOG_INFO("Cancelled {} timer from tempo broadcaster", num_cancelled);
}

void BroadcastLoop::do_broadcast() {
  DataBuffer::Ptr response_buffer_ptr = prepare_buffer_();
  SPDLOG_DEBUG("Broadcasting: {::x}", *response_buffer_ptr);

  socket_->async_send_to(
      asio::buffer(response_buffer_ptr->data(), response_buffer_ptr->size()),
      broadcast_address_,
      [this](std::error_code ec, std::size_t l /*bytes_sent*/) {
        if (ec) {
          SPDLOG_ERROR("Error broadcasting tempo {}", ec.message());

          return;
        }

        // Need to check if we haven't passed beyond next time.
        auto next_time = timer_.expiry();
        while (next_time < std::chrono::high_resolution_clock::now()) {
          next_time += alarm_period_;
        }

        timer_.expires_at(next_time);
        timer_.async_wait([this](const asio::error_code &error) {
          if (error) {
            if (error == asio::error::operation_aborted) {
              SPDLOG_INFO("Timer cancelled");
            } else {
              SPDLOG_ERROR("Error with timer callback {}", error.message());
            }
            return;
          }

          do_broadcast();
        });
      });
}
} // namespace beatled::server