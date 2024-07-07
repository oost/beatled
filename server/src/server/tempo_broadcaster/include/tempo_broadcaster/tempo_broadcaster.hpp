#ifndef SERVER_TEMPO_BROADCASTER_H
#define SERVER_TEMPO_BROADCASTER_H

#include <asio.hpp>
#include <chrono>

#include "core/interfaces/service_controller.hpp"
#include "core/state_manager.hpp"
#include "udp/udp_buffer.hpp"

using beatled::core::ServiceControllerInterface;
using beatled::core::StateManager;

namespace beatled::server {

class BroadcastLoop;

class TempoBroadcaster : public ServiceControllerInterface {
public:
  struct parameters_t {
    const std::string address;
    std::uint16_t port;
  };

  TempoBroadcaster(const std::string &id, asio::io_context &io_context,
                   std::chrono::nanoseconds alarm_period,
                   std::chrono::nanoseconds program_alarm_period,
                   const parameters_t &broadcasting_server_parameters,
                   StateManager &state_manager);

  ~TempoBroadcaster();

  void start_sync() override;
  void stop_sync() override;

  void broadcast_next_beat(uint64_t next_beat_time_ref, uint32_t beat_count);
  void broadcast_beat(uint64_t beat_time_ref, uint32_t beat_count);

private:
  const char *SERVICE_NAME = "Tempo Broadcaster";
  const char *service_name() const override { return SERVICE_NAME; }

  void do_broadcast_beat();
  void do_broadcast_program();
  void do_broadcast(const char *sendBuf, uint16_t sendBuf_l,
                    asio::high_resolution_timer &timer,
                    const std::chrono::nanoseconds &alarm_period,
                    std::function<void(void)> callback);

  void send_response(DataBuffer::Ptr &&response_buffer);

  StateManager &state_manager_;
  asio::io_context &io_context_;

  std::chrono::nanoseconds alarm_period_;

  asio::high_resolution_timer program_timer_;
  std::chrono::nanoseconds program_alarm_period_;

  std::shared_ptr<asio::ip::udp::socket> socket_;
  asio::ip::udp::endpoint broadcast_endpoint_;

  int count_;
  std::uint16_t port_;
  std::uint8_t led_program;

  std::vector<std::unique_ptr<BroadcastLoop>> loops_;
  int program_idx_;
  const parameters_t &broadcasting_server_parameters_;

  asio::strand<asio::io_context::executor_type> strand_;
};
} // namespace beatled::server

#endif // SERVER_TEMPO_BROADCASTER_H