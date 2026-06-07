#ifndef SERVER_SRC_CONFIG_INCLUDE_CONFIG_CONFIG_HPP_
#define SERVER_SRC_CONFIG_INCLUDE_CONFIG_CONFIG_HPP_

#include <map>
#include <memory>
#include <string>
#include <thread>

#include <fmt/format.h>
#include <iostream>
#include <lyra/lyra.hpp>

#include "server/server.hpp"

namespace beatled::core {

class Config {
public:
  Config(int argc, const char *argv[]);

  server::Server::parameters_t server_parameters() const;
  void log_config() const;
  bool help() const { return m_help; }

private:
  bool m_help{false};
  std::string m_address{"localhost"};
  bool m_start_http_server = false;
  bool m_start_udp_server = false;
  bool m_start_broadcaster = false;
  bool m_no_tls{false};
  std::uint16_t m_http_port{8443};
  std::uint16_t m_udp_port{9090};
  std::string m_broadcasting_address{"255.255.255.255"};
  std::uint16_t m_broadcasting_port{8765};
  // limited | subnet | unicast (default unicast — most reliable on Wi-Fi).
  std::string m_broadcast_mode{"unicast"};
  std::size_t m_pool_size{2};
  std::string m_root_dir{"."};
  std::string m_certs_dir{"./certs"};
  std::string m_cors_origin;
  std::string m_api_token;
  // spdlog level: trace / debug / info / warn / err / critical / off.
  // Resolved from --log-level (highest priority) → BEATLED_LOG_LEVEL env
  // → built-in default "info".
  std::string m_log_level{"info"};
  // Background PROGRAM refresh period. Lower = controllers that missed
  // the on-change push catch up faster; the cost is one 5-byte UDP
  // packet per registered client per refresh tick.
  std::uint32_t m_program_refresh_ms{200};
  // Active STATUS probe period (protocol v4). The broadcaster fires a
  // STATUS_REQUEST at every registered client every N ms; controllers
  // reply with STATUS_RESPONSE which the server folds into
  // ClientStatus::latest_qos. 0 disables the probe entirely.
  std::uint32_t m_status_probe_ms{5000};
  // QoS health-pip thresholds, in microseconds. Fleet skew (max-min
  // controller offset) at or above warn turns the Fleet QoS pip amber;
  // at or above fail turns it red. Total intercore-drop / time-sync
  // outlier counts also turn the pip red regardless of skew.
  std::uint32_t m_qos_skew_warn_us{5000};
  std::uint32_t m_qos_skew_fail_us{20000};
};

} // namespace beatled::core

#endif // SERVER_SRC_CONFIG_INCLUDE_CONFIG_CONFIG_HPP_