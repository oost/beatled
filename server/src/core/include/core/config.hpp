#ifndef SERVER_SRC_CONFIG_INCLUDE_CONFIG_CONFIG_HPP_
#define SERVER_SRC_CONFIG_INCLUDE_CONFIG_CONFIG_HPP_

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <thread>

#include <fmt/format.h>
#include <iostream>
#include <lyra/lyra.hpp>

namespace beatled::core {

// Holds the parsed command-line configuration. It deliberately knows nothing
// about the server target: the mapping from these values to a
// server::Server::parameters_t lives in the server layer
// (server::make_server_parameters), which keeps beatled_core free of any
// dependency on beatled_server.
class Config {
public:
  Config(int argc, const char *argv[]);

  void log_config() const;
  bool help() const { return m_help; }

  // Accessors consumed by server::make_server_parameters().
  bool start_http_server() const { return m_start_http_server; }
  bool start_udp_server() const { return m_start_udp_server; }
  bool start_broadcaster() const { return m_start_broadcaster; }
  bool no_tls() const { return m_no_tls; }
  const std::string &address() const { return m_address; }
  const std::string &root_dir() const { return m_root_dir; }
  const std::string &certs_dir() const { return m_certs_dir; }
  const std::string &cors_origin() const { return m_cors_origin; }
  const std::string &api_token() const { return m_api_token; }
  const std::string &broadcasting_address() const { return m_broadcasting_address; }
  const std::string &broadcast_mode() const { return m_broadcast_mode; }
  const std::string &log_level() const { return m_log_level; }
  std::uint16_t http_port() const { return m_http_port; }
  std::uint16_t udp_port() const { return m_udp_port; }
  std::uint16_t broadcasting_port() const { return m_broadcasting_port; }
  std::size_t pool_size() const { return m_pool_size; }
  std::uint32_t program_refresh_ms() const { return m_program_refresh_ms; }
  std::uint32_t status_probe_ms() const { return m_status_probe_ms; }
  std::uint32_t qos_skew_warn_us() const { return m_qos_skew_warn_us; }
  std::uint32_t qos_skew_fail_us() const { return m_qos_skew_fail_us; }

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