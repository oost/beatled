#include <asio.hpp>
#include <iostream>

#include "../../state_manager/state_manager.hpp"
#include "broadcast_loop.hpp"

using asio::ip::udp;

using namespace server;

BroadcastLoop::BroadcastLoop(
    std::shared_ptr<asio::ip::udp::socket> socket,
    std::chrono::nanoseconds alarm_period,
    std::function<char *(void)> prepare_buffer,
    std::function<void(char *)> dispose_buffer,
    const broadcasting_server_parameters_t &broadcasting_server_parameters)
    : alarm_period_(alarm_period), count_(0),
      timer_(asio::high_resolution_timer(socket->get_executor(), alarm_period)),
      socket_(socket), broadcast_address_(asio::ip::make_address_v4(
                           broadcasting_server_parameters.address)),
      port_(broadcasting_server_parameters.port),
      prepare_buffer_(prepare_buffer), dispose_buffer_(dispose_buffer) {

  std::cout << "Starting Broadcast loop " << std::endl;

  do_broadcast();
}

void BroadcastLoop::do_broadcast() {

  udp::endpoint broadcast_address(udp::endpoint(broadcast_address_, port_));

  char *sendBuf = prepare_buffer_();
  std::cout << "Broadcasting: " << (int)sendBuf[0] << " to "
            << broadcast_address << std::endl;

  socket_->async_send_to(
      asio::buffer(sendBuf, 1), broadcast_address,
      [this, sendBuf](std::error_code /*ec*/, std::size_t l /*bytes_sent*/) {
        dispose_buffer_(sendBuf);
        // Need to check if we haven't passed beyond next time.
        auto next_time = timer_.expiry();
        while (next_time < std::chrono::high_resolution_clock::now()) {
          next_time += alarm_period_;
        }

        timer_.expires_at(next_time);
        timer_.async_wait([this](auto) { do_broadcast(); });
      });
}
