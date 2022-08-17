#ifndef BROADCAST_LOOP_H
#define BROADCAST_LOOP_H

#include <asio.hpp>
#include <chrono>

#include "server_parameters.hpp"
namespace server {

class BroadcastLoop {
public:
  BroadcastLoop(
      std::shared_ptr<asio::ip::udp::socket>,
      std::chrono::nanoseconds alarm_period,
      std::function<std::string(void)> prepare_buffer,
      const broadcasting_server_parameters_t &broadcasting_server_parameters);

private:
  void do_broadcast();
  int count_;
  std::shared_ptr<asio::ip::udp::socket> socket_;
  asio::high_resolution_timer timer_;
  std::chrono::nanoseconds alarm_period_;
  std::function<std::string(void)> prepare_buffer_;
  asio::ip::udp::endpoint broadcast_address_;
};

} // namespace server
#endif // BROADCAST_LOOP_H