#include <asio.hpp>
#include <fmt/ostream.h>
#include <iostream>
#include <spdlog/spdlog.h>

#include "broadcast_loop.hpp"
#include "core/state_manager.hpp"
#include "tempo_broadcaster/tempo_broadcaster.hpp"
#include "udp/udp_buffer.hpp"

namespace beatled::server {

using asio::ip::udp;
using beatled::core::StateManager;
using beatled::core::tempo_ref_t;

TempoBroadcaster::TempoBroadcaster(
    const std::string &id, asio::io_context &io_context,
    std::chrono::nanoseconds alarm_period,
    std::chrono::nanoseconds program_alarm_period,
    const parameters_t &broadcasting_server_parameters,
    StateManager &state_manager)
    : ServiceControllerInterface{id}, io_context_(io_context),
      alarm_period_(alarm_period), program_alarm_period_(program_alarm_period),
      count_(0), program_timer_(asio::high_resolution_timer(
                     io_context_, program_alarm_period_)),
      strand_{asio::make_strand(io_context)},
      program_idx_(0), state_manager_{state_manager},
      broadcast_endpoint_{udp::endpoint(
          asio::ip::make_address_v4(broadcasting_server_parameters.address),
          broadcasting_server_parameters.port)},
      socket_(std::make_shared<asio::ip::udp::socket>(io_context)),
      broadcasting_server_parameters_{broadcasting_server_parameters} {
  SPDLOG_INFO("Creating {}", name());

  socket_->open(udp::v4());
  if (!socket_->is_open()) {
    throw std::runtime_error("Can't open UDP socket");
  }

  socket_->set_option(udp::socket::reuse_address(true));
  socket_->set_option(asio::socket_base::broadcast(true));

  // socket_->bind(udp::endpoint(broadcast_address_, port_));
  SPDLOG_INFO("{} broadcasting on UDP through {} {}", name(),
              fmt::streamed(socket_->local_endpoint()),
              fmt::streamed(broadcast_endpoint_));

  // state_manager_.register_next_beat_cb([this](uint64_t next_beat_time_ref) {
  //   broadcast_next_beat(next_beat_time_ref);
  // });
}

TempoBroadcaster::~TempoBroadcaster() {}

void TempoBroadcaster::broadcast_next_beat(uint64_t next_beat_time_ref,
                                           uint32_t beat_count) {
  if (!is_running()) {
    SPDLOG_INFO("TempoBroadcaster is not on. Dropping next beat broadcast.");
    return;
  }

  asio::post(strand_, [this, next_beat_time_ref, beat_count]() {
    tempo_ref_t tr = state_manager_.get_tempo_ref();
    uint16_t pid = state_manager_.get_program_id();

    DataBuffer::Ptr response_buffer = std::make_unique<NextBeatBuffer>(
        next_beat_time_ref, tr.tempo_period_us, beat_count, pid);

    SPDLOG_INFO("{} broadcasting next beat on UDP through {}->{}", name(),
                fmt::streamed(socket_->local_endpoint()),
                fmt::streamed(broadcast_endpoint_));

    this->send_response(std::move(response_buffer));
    // // Broadcast
    // socket_->async_send_to(
    //     asio::buffer(response_buffer->data(), response_buffer->size()),
    //     broadcast_endpoint_,
    //     asio::bind_executor(
    //         strand_, [this](std::error_code ec, std::size_t l /*bytes_sent*/)
    //         {
    //           if (ec) {
    //             SPDLOG_ERROR("Error broadcasting tempo {}", ec.message());
    //             return;
    //           }
    //         }));
  });
}

void TempoBroadcaster::broadcast_beat(uint64_t beat_time_ref,
                                      uint32_t beat_count) {
  if (!is_running()) {
    SPDLOG_INFO("TempoBroadcaster is not on. Dropping beat broadcast.");
    return;
  }

  asio::post(strand_, [this, beat_time_ref, beat_count]() {
    tempo_ref_t tr = state_manager_.get_tempo_ref();
    uint16_t pid = state_manager_.get_program_id();

    DataBuffer::Ptr response_buffer = std::make_unique<BeatBuffer>(
        beat_time_ref, tr.tempo_period_us, beat_count, pid);

    SPDLOG_INFO("{} broadcasting beat on UDP through {}->{}", name(),
                fmt::streamed(socket_->local_endpoint()),
                fmt::streamed(broadcast_endpoint_));

    this->send_response(std::move(response_buffer));
    // // Broadcast
    // socket_->async_send_to(
    //     asio::buffer(response_buffer->data(), response_buffer->size()),
    //     broadcast_endpoint_,
    //     asio::bind_executor(
    //         strand_, [this](std::error_code ec, std::size_t l /*bytes_sent*/)
    //         {
    //           if (ec) {
    //             SPDLOG_ERROR("Error broadcasting tempo {}", ec.message());
    //             return;
    //           }
    //         }));
  });
}

void TempoBroadcaster::send_response(DataBuffer::Ptr &&response_buffer) {
  // Broadcast
  socket_->async_send_to(
      asio::buffer(response_buffer->data(), response_buffer->size()),
      broadcast_endpoint_,
      asio::bind_executor(
          strand_, [this](std::error_code ec, std::size_t l /*bytes_sent*/) {
            if (ec) {
              SPDLOG_ERROR("Error broadcasting tempo {}", ec.message());
              return;
            }
          }));
}

void TempoBroadcaster::start_sync() {}

void TempoBroadcaster::stop_sync() { socket_->cancel(); }

// void TempoBroadcaster::start_sync() {
//   loops_.push_back(std::make_unique<BroadcastLoop>(
//       socket_, alarm_period_,
//       asio::bind_executor(strand_,
//                           [this]() {
//                             tempo_ref_t tr = state_manager_.get_tempo_ref();
//                             uint16_t pid = state_manager_.get_program_id();

//                             DataBuffer::Ptr response_buffer =
//                                 std::make_unique<TempoResponseBuffer>(
//                                     tr.beat_time_ref, tr.tempo_period_us,
//                                     pid);
//                             return std::move(response_buffer);
//                           }),
//       broadcasting_server_parameters_));
// }

// void TempoBroadcaster::stop_sync() {
//   socket_->cancel();
//   loops_.clear();
// }
} // namespace beatled::server