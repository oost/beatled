#include <cstdlib>
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
      lyra::opt(m_verbose)["--verbose"]("Enable debug logging");

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
}

void Config::log_config() const {
  SPDLOG_INFO("Configuration:");
  SPDLOG_INFO("  Address:            {}", m_address);
  SPDLOG_INFO("  HTTP server:        {} (port {}{})", m_start_http_server ? "on" : "off", m_http_port,
              m_no_tls ? ", no TLS" : "");
  SPDLOG_INFO("  UDP server:         {} (port {})", m_start_udp_server ? "on" : "off", m_udp_port);
  SPDLOG_INFO("  Broadcaster:        {} ({}:{})", m_start_broadcaster ? "on" : "off",
              m_broadcasting_address, m_broadcasting_port);
  SPDLOG_INFO("  Thread pool size:   {}", m_pool_size);
  SPDLOG_INFO("  Root dir:           {}", m_root_dir);
  SPDLOG_INFO("  Certs dir:          {}", m_certs_dir);
  SPDLOG_INFO("  CORS origin:        {}", m_cors_origin.empty() ? "disabled" : m_cors_origin);
  SPDLOG_INFO("  API token:          {}", m_api_token.empty() ? "disabled" : "set");
}

beatled::server::Server::parameters_t Config::server_parameters() const {
  server::Server::parameters_t server_parameters{
      .start_http_server = m_start_http_server,
      .start_udp_server = m_start_udp_server,
      .start_broadcaster = m_start_broadcaster,
      .http =
          {
              m_address,     // address
              m_http_port,   // port
              m_root_dir,    // root_dir
              m_certs_dir,   // cert_dir
              m_cors_origin, // cors_origin
              m_api_token,   // api_token
              m_no_tls,      // no_tls
          },
      .udp = {m_udp_port},
      .broadcasting = {m_broadcasting_address, m_broadcasting_port},
      .logger = {20, m_verbose},
      .thread_pool_size = m_pool_size,
  };

  return server_parameters;
}