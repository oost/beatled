#include <cstdlib>
#include <stdexcept>

#include <fmt/ostream.h>
#include <spdlog/spdlog.h>

#include "core/config.hpp"

using namespace beatled::core;

Config::Config(int argc, const char *argv[]) {

  auto cli =
      lyra::help(m_help) |
      lyra::opt(m_address, "address")["-a"]["--address"](
          fmt::format("address to listen (default: {})", m_address)) |
      lyra::opt(m_start_http_server)["--start-http"]("Start HTTP server") |
      lyra::opt(m_start_udp_server)["--start-udp"]("Start UDP server") |
      lyra::opt(m_start_broadcaster)["--start-broadcast"]("Start broadcaster ") |
      lyra::opt(m_no_tls)["--no-tls"]("Disable TLS (plain HTTP)") |

      lyra::opt(m_http_port, "http port")["-p"]["--http-port"](
          fmt::format("port to listen (default: {})", m_http_port)) |
      lyra::opt(m_udp_port, "udp port")["-u"]["--udp-port"](
          fmt::format("port to listen (default: {})", m_udp_port)) |
      lyra::opt(m_broadcasting_address, "broadcasting address")["-c"]["--m_broadcasting-address"](
          fmt::format("port to listen (default: {})", m_broadcasting_address)) |
      lyra::opt(m_broadcasting_port, "broadcasting port")["-b"]["--broadcasting-port"](
          fmt::format("port to listen (default: {})", m_broadcasting_port)) |
      lyra::opt(m_broadcast_mode, "limited|subnet|unicast")["--broadcast-mode"](
          fmt::format("delivery mode for NEXT_BEAT/BEAT/PROGRAM (default: {})", m_broadcast_mode)) |
      lyra::opt(m_pool_size, "thread-pool size")["-n"]["--thread-pool-size"](
          fmt::format("The size of a thread pool to run server (default: {})", m_pool_size)) |
      lyra::opt(m_root_dir, "root-dir")["-r"]["--root-dir"](
          fmt::format("server root dir (default: '{}')", m_root_dir)) |
      lyra::opt(m_certs_dir, "certs dir")["--certs-dir"](
          fmt::format("server certs dir (default: '{}')", m_certs_dir)) |
      lyra::opt(m_cors_origin,
                "cors origin")["--cors-origin"]("CORS allowed origin (default: disabled)") |
      lyra::opt(m_api_token, "api token")["--api-token"](
          "Bearer token for API authentication (default: disabled)") |
      lyra::opt(m_log_level, "trace|debug|info|warn|err|critical|off")["--log-level"](
          fmt::format("spdlog level (default: {}; also: BEATLED_LOG_LEVEL env var)", m_log_level)) |
      lyra::opt(m_program_refresh_ms, "ms")["--program-refresh-ms"](fmt::format(
          "PROGRAM background refresh period in ms (default: {})", m_program_refresh_ms)) |
      lyra::opt(m_status_probe_ms, "ms")["--status-probe-ms"](
          fmt::format("STATUS probe period in ms; 0 disables (default: {})", m_status_probe_ms)) |
      lyra::opt(m_qos_skew_warn_us, "us")["--qos-skew-warn-us"](
          fmt::format("Fleet skew (max-min controller offset) above which the QoS pip turns amber "
                      "(default: {} us)",
                      m_qos_skew_warn_us)) |
      lyra::opt(m_qos_skew_fail_us, "us")["--qos-skew-fail-us"](fmt::format(
          "Fleet skew above which the QoS pip turns red (default: {} us)", m_qos_skew_fail_us));

  auto parser_result = cli.parse(lyra::args(argc, argv));
  if (!parser_result) {
    throw std::runtime_error{
        fmt::format("Invalid command-line arguments: {}", parser_result.message())};
  }

  if (m_help) {
    SPDLOG_INFO(fmt::streamed(cli));
  }

  // Fall back to environment variable if no CLI token was provided
  if (m_api_token.empty()) {
    const char *env_token = std::getenv("BEATLED_API_TOKEN");
    if (env_token) {
      m_api_token = env_token;
    }
  }
  // Log level: CLI wins over env wins over default. The CLI default is
  // "info"; treat the env var as the source-of-truth only when the user
  // didn't override on the command line.
  if (m_log_level == "info") {
    const char *env_level = std::getenv("BEATLED_LOG_LEVEL");
    if (env_level && *env_level) {
      m_log_level = env_level;
    }
  }
}

void Config::log_config() const {
  SPDLOG_INFO("Configuration:");
  SPDLOG_INFO("  Address:            {}", m_address);
  SPDLOG_INFO("  HTTP server:        {} (port {}{})", m_start_http_server ? "on" : "off",
              m_http_port, m_no_tls ? ", no TLS" : "");
  SPDLOG_INFO("  UDP server:         {} (port {})", m_start_udp_server ? "on" : "off", m_udp_port);
  SPDLOG_INFO("  Broadcaster:        {} ({}:{}, mode={})", m_start_broadcaster ? "on" : "off",
              m_broadcasting_address, m_broadcasting_port, m_broadcast_mode);
  SPDLOG_INFO("  Thread pool size:   {}", m_pool_size);
  SPDLOG_INFO("  Root dir:           {}", m_root_dir);
  SPDLOG_INFO("  Certs dir:          {}", m_certs_dir);
  SPDLOG_INFO("  CORS origin:        {}", m_cors_origin.empty() ? "disabled" : m_cors_origin);
  SPDLOG_INFO("  API token:          {}", m_api_token.empty() ? "disabled" : "set");
  SPDLOG_INFO("  Log level:          {}", m_log_level);
  SPDLOG_INFO("  PROGRAM refresh:    {} ms", m_program_refresh_ms);
  SPDLOG_INFO("  STATUS probe:       {}", m_status_probe_ms == 0
                                              ? std::string("off")
                                              : fmt::format("{} ms", m_status_probe_ms));
  SPDLOG_INFO("  QoS skew warn/fail: {} us / {} us", m_qos_skew_warn_us, m_qos_skew_fail_us);
}