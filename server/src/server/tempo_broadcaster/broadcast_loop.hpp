#ifndef BROADCAST_LOOP_H
#define BROADCAST_LOOP_H

#include <asio.hpp>
#include <chrono>

#include "tempo_broadcaster/tempo_broadcaster.hpp"
#include "udp/udp_buffer.hpp"

namespace beatled::server {

class BroadcastLoop {
public:
  BroadcastLoop(
      std::shared_ptr<asio::ip::udp::socket>,
      std::chrono::nanoseconds alarm_period,
      std::function<DataBuffer::Ptr(void)> prepare_buffer,
      const TempoBroadcaster::parameters_t &broadcasting_server_parameters);
  ~BroadcastLoop();

private:
  void do_broadcast();
  int count_;
  std::shared_ptr<asio::ip::udp::socket> socket_;
  asio::high_resolution_timer timer_;
  std::chrono::nanoseconds alarm_period_;
  std::function<DataBuffer::Ptr(void)> prepare_buffer_;
  asio::ip::udp::endpoint broadcast_address_;
};

} // namespace beatled::server

#endif // BROADCAST_LOOP_H