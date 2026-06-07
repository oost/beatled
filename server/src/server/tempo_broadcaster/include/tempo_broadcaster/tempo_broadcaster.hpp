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

// Delivery mode for the broadcaster. See --broadcast-mode CLI flag.
//
// - Limited:  send to 255.255.255.255. Often dropped by consumer Wi-Fi APs.
// - Subnet:   send to the broadcast address configured via --broadcast-address
//             (e.g. 192.168.1.255). Forwarded more reliably than limited.
// - Unicast:  send one packet per registered client to their last-known
//             endpoint, with per-client OWD compensation applied to
//             NEXT_BEAT timestamps. Preferred when client count is modest.
enum class BroadcastMode { Limited, Subnet, Unicast };

class TempoBroadcaster : public ServiceControllerInterface {
public:
  struct parameters_t {
    const std::string address;
    std::uint16_t port;
    BroadcastMode mode = BroadcastMode::Limited;
  };

  TempoBroadcaster(const std::string &id, asio::io_context &io_context,
                   std::chrono::nanoseconds program_refresh_period,
                   std::chrono::nanoseconds status_probe_period,
                   const parameters_t &broadcasting_server_parameters, StateManager &state_manager);

  ~TempoBroadcaster();

  void start_sync() override;
  void stop_sync() override;

  void broadcast_next_beat(uint64_t next_beat_time_ref, uint32_t beat_count);
  void broadcast_beat(uint64_t beat_time_ref, uint32_t beat_count);

  // Push the current program_id immediately. Called from the StateManager
  // program-change callback so controllers see new programs without waiting
  // for the next refresh tick.
  void push_program_now();

private:
  const char *SERVICE_NAME = "Tempo Broadcaster";
  const char *service_name() const override { return SERVICE_NAME; }

  // Send a single buffer either as broadcast or as N unicast frames.
  void dispatch(DataBuffer::Ptr response_buffer, bool compensate_owd,
                uint64_t base_time_for_compensation);

  void send_to_endpoint(std::shared_ptr<DataBuffer> buffer,
                        const asio::ip::udp::endpoint &endpoint);

  void schedule_program_refresh();

  // Server-initiated STATUS probe (protocol v4). Fires a unicast
  // STATUS_REQUEST to every registered client every `status_probe_period`;
  // controllers reply with STATUS_RESPONSE which the request handler
  // decodes back onto ClientStatus::latest_qos. status_probe_period == 0
  // disables the probe entirely.
  void schedule_status_probe();
  void send_status_probes();

  StateManager &state_manager_;
  asio::io_context &io_context_;

  asio::high_resolution_timer program_refresh_timer_;
  std::chrono::nanoseconds program_refresh_period_;
  asio::high_resolution_timer status_probe_timer_;
  std::chrono::nanoseconds status_probe_period_;

  std::shared_ptr<asio::ip::udp::socket> socket_;
  asio::ip::udp::endpoint broadcast_endpoint_;

  const parameters_t broadcasting_server_parameters_;

  asio::strand<asio::io_context::executor_type> strand_;

  // Monotonic sequence numbers per message kind. Wrap is acceptable; the
  // controller uses 16-bit modular arithmetic to detect gaps.
  std::atomic<uint16_t> next_beat_seq_{0};
  std::atomic<uint16_t> beat_seq_{0};
  std::atomic<uint16_t> program_seq_{0};
};
} // namespace beatled::server

#endif // SERVER_TEMPO_BROADCASTER_H