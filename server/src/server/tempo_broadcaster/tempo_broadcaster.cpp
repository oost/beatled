#include <asio.hpp>
#include <iostream>

#include "../../state_manager/state_manager.hpp"
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
      socket_(io_context), broadcast_address_(asio::ip::make_address_v4(
                               broadcasting_server_parameters.address)),
      port_(broadcasting_server_parameters.port) {

  std::cout << "Starting TempoBroadcaster " << std::endl;

  socket_.open(udp::v4());
  if (!socket_.is_open()) {
    throw std::runtime_error("Can't open UDP socket");
  }

  socket_.set_option(udp::socket::reuse_address(true));
  socket_.set_option(asio::socket_base::broadcast(true));

  // socket_.bind(udp::endpoint(broadcast_address_, port_));
  std::cout << "TempoBroadcaster broadcasting on UDP through "
            << socket_.local_endpoint() << " " << broadcast_address_
            << std::endl;

  do_broadcast_program();
  // do_broadcast_beat();
}

void TempoBroadcaster::do_broadcast_beat() {
  count_++;
  udp::endpoint broadcast_address(udp::endpoint(broadcast_address_, port_));

  // precise_time_t time_ref; //= StateManager::get_instance()->get_time_ref();
  // float tempo;             //= StateManager::get_instance()->get_tempo();
  // std::cout << "Broadcasting tempo: " << tempo
  //           << " timeref: " << time_ref.tv_sec << std::endl;

  char *sendBuf = new char[1];
  sendBuf[0] = 4;

  // sendBuf[0] = 2;
  // sendBuf[1] = (uint32_t)tempo >> 24;
  // sendBuf[2] = (uint32_t)tempo >> 16;
  // sendBuf[3] = (uint32_t)tempo >> 8;
  // sendBuf[4] = (uint32_t)tempo;
  // sendBuf[5] = time_ref.tv_sec >> 24;
  // sendBuf[6] = time_ref.tv_sec >> 16;
  // sendBuf[7] = time_ref.tv_sec >> 8;
  // sendBuf[8] = time_ref.tv_sec;
  // sendBuf[9] = '\0';

  std::cout << "Broadcasting: " << (int)sendBuf[0] << " to "
            << broadcast_address << std::endl;

  socket_.async_send_to(
      asio::buffer(sendBuf, 1), broadcast_address,
      [this, sendBuf](std::error_code /*ec*/, std::size_t /*bytes_sent*/) {
        delete[] sendBuf;
        // Need to check if we haven't passed beyond next time.
        auto next_time = timer_.expiry();
        while (next_time < std::chrono::high_resolution_clock::now()) {
          next_time += alarm_period_;
        }

        timer_.expires_at(next_time);
        timer_.async_wait([this](auto) { this->do_broadcast_beat(); });
      });
}

void do_broadcast(const char *sendBuf, uint16_t sendBuf_l,
                  asio::high_resolution_timer &timer,
                  const std::chrono::nanoseconds &alarm_period;
                  std::function<void(void)> callback) {
  count_++;
  udp::endpoint broadcast_address(udp::endpoint(broadcast_address_, port_));

  std::cout << "Broadcasting: " << (int)sendBuf[0] << " to "
            << broadcast_address << std::endl;

  socket_.async_send_to(
      asio::buffer(sendBuf, 1), broadcast_address,
      [this, sendBuf](std::error_code /*ec*/, std::size_t l /*bytes_sent*/) {
        // Need to check if we haven't passed beyond next time.
        auto next_time = timer.expiry();
        while (next_time < std::chrono::high_resolution_clock::now()) {
          next_time += alarm_period;
        }

        timer.expires_at(next_time);
        timer.async_wait([this](auto) { callback(); });
      });
}

void TempoBroadcaster::do_broadcast_program() {
  udp::endpoint broadcast_address(udp::endpoint(broadcast_address_, port_));

  char *sendBuf = new char[2];
  sendBuf[0] = 3;
  sendBuf[1] = 1;

  std::cout << "Broadcasting Program: " << (int)sendBuf[0] << (int)sendBuf[1]
            << " to " << broadcast_address << std::endl;

  socket_.async_send_to(
      asio::buffer(sendBuf, 2), broadcast_address,
      [this, sendBuf](std::error_code /*ec*/, std::size_t l /*bytes_sent*/) {
        std::cout << "Sent " << l << std::endl;
        delete[] sendBuf;
        // Need to check if we haven't passed beyond next time.
        auto next_time = program_timer_.expiry();
        // while (next_time < std::chrono::high_resolution_clock::now()) {
        next_time += program_alarm_period_;
        // }

        program_timer_.expires_at(next_time);
        program_timer_.async_wait([this](auto) {
          std::cout << "Timer !!  " << std::endl;

          this->do_broadcast_program();
        });
      });
}
