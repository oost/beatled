#include <asio.hpp>
#include <iostream>

#include "../../state_manager/state_manager.hpp"
#include "broadcast_loop.hpp"

using asio::ip::udp;

using namespace server;

BroadcastLoop::BroadcastLoop(
    std::shared_ptr<asio::ip::udp::socket> socket,
    std::chrono::nanoseconds alarm_period,
    std::function<UDPResponseBuffer::Ptr(void)> prepare_buffer,
    const broadcasting_server_parameters_t &broadcasting_server_parameters)
    : alarm_period_{alarm_period}, count_{0},
      timer_{asio::high_resolution_timer(socket->get_executor(), alarm_period)},
      socket_{socket}, prepare_buffer_{prepare_buffer},
      broadcast_address_{udp::endpoint(
          asio::ip::make_address_v4(broadcasting_server_parameters.address),
          broadcasting_server_parameters.port)} {
  std::cout << "Starting Broadcast loop " << std::endl;

  do_broadcast();
}

void BroadcastLoop::do_broadcast() {
  UDPResponseBuffer::Ptr response_buffer_ptr = prepare_buffer_();
  std::cout << "Broadcasting: " << *response_buffer_ptr << std::endl;

  socket_->async_send_to(
      asio::buffer(response_buffer_ptr->data(), response_buffer_ptr->size()),
      broadcast_address_,
      [this](std::error_code /*ec*/, std::size_t l /*bytes_sent*/) {
        std::cout << "Broadcasted: bytes sent=" << std::dec << l << std::endl;

        // Need to check if we haven't passed beyond next time.
        auto next_time = timer_.expiry();
        while (next_time < std::chrono::high_resolution_clock::now()) {
          next_time += alarm_period_;
        }

        timer_.expires_at(next_time);
        timer_.async_wait([this](auto) { do_broadcast(); });
      });
}
