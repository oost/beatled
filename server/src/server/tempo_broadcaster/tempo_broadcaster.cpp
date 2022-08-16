#include <asio.hpp>
#include <iostream>

#include "../../state_manager/state_manager.hpp"
#include "tempo_broadcaster.hpp"

using asio::ip::udp;

TempoBroadcaster::TempoBroadcaster(asio::io_context &io_context,
                                   std::chrono::nanoseconds alarm_period,
                                   short port)
    : io_context_(io_context), alarm_period_(alarm_period), count_(0),
      timer_(asio::high_resolution_timer(io_context_, alarm_period)),
      socket_(io_context, udp::endpoint(udp::v4(), port)), port_(port) {

  std::cout << "TempoBroadcaster broadcasting on UDP through "
            << socket_.local_endpoint() << std::endl;

  socket_.set_option(udp::socket::reuse_address(true));
  socket_.set_option(asio::socket_base::broadcast(true));

  do_broadcast();
}

void TempoBroadcaster::do_broadcast() {
  count_++;
  precise_time_t time_ref; //= StateManager::get_instance()->get_time_ref();
  float tempo;             //= StateManager::get_instance()->get_tempo();
  std::cout << "Broadcasting tempo: " << tempo
            << " timeref: " << time_ref.tv_sec << std::endl;

  std::shared_ptr<std::uint8_t[]> sendBuf(new std::uint8_t[9]);
  sendBuf[0] = 2;
  sendBuf[1] = (uint32_t)tempo >> 24;
  sendBuf[2] = (uint32_t)tempo >> 16;
  sendBuf[3] = (uint32_t)tempo >> 8;
  sendBuf[4] = (uint32_t)tempo;
  sendBuf[5] = time_ref.tv_sec >> 24;
  sendBuf[6] = time_ref.tv_sec >> 16;
  sendBuf[7] = time_ref.tv_sec >> 8;
  sendBuf[8] = time_ref.tv_sec;

  socket_.async_send_to(
      asio::buffer(sendBuf.get(), 9),
      udp::endpoint(asio::ip::address_v4::broadcast(), port_),
      [this, sendBuf](std::error_code /*ec*/, std::size_t /*bytes_sent*/) {
        // Need to check if we haven't passed beyond next time.
        auto next_time = timer_.expiry();
        while (next_time < std::chrono::high_resolution_clock::now()) {
          next_time += alarm_period_;
        }

        timer_.expires_at(next_time);
        timer_.async_wait([this](auto) { do_broadcast(); });
      });
}