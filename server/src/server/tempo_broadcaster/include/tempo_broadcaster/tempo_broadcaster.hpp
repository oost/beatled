#ifndef SERVER_TEMPO_BROADCASTER_H
#define SERVER_TEMPO_BROADCASTER_H

#include <asio.hpp>
#include <chrono>

#include "broadcast_loop.hpp"
#include "server_parameters.hpp"
#include "state_manager/state_manager.hpp"

namespace server {
class TempoBroadcaster {
public:
  TempoBroadcaster(
      asio::io_context &io_context, std::chrono::nanoseconds alarm_period,
      std::chrono::nanoseconds program_alarm_period,
      const broadcasting_server_parameters_t &broadcasting_server_parameters,
      StateManager &state_manager);

private:
  void do_broadcast_beat();
  void do_broadcast_program();
  void do_broadcast(const char *sendBuf, uint16_t sendBuf_l,
                    asio::high_resolution_timer &timer,
                    const std::chrono::nanoseconds &alarm_period,
                    std::function<void(void)> callback);

  StateManager &state_manager_;
  asio::io_context &io_context_;

  asio::high_resolution_timer timer_;
  std::chrono::nanoseconds alarm_period_;

  asio::high_resolution_timer program_timer_;
  std::chrono::nanoseconds program_alarm_period_;

  std::shared_ptr<asio::ip::udp::socket> socket_;
  asio::ip::address_v4 broadcast_address_;
  int count_;
  std::uint16_t port_;
  std::uint8_t led_program;

  std::vector<std::unique_ptr<BroadcastLoop>> loops_;
  int program_idx_;
};
} // namespace server

#endif // SERVER_TEMPO_BROADCASTER_H