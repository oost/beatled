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
      lyra::opt(m_start_broadcaster)["--start-broadcast"](
          "Start broadcaster ") |

      lyra::opt(m_http_port, "http port")["-p"]["--http-port"](
          fmt::format("port to listen (default: {})", m_http_port)) |
      lyra::opt(m_udp_port, "udp port")["-u"]["--udp-port"](
          fmt::format("port to listen (default: {})", m_udp_port)) |
      lyra::opt(m_broadcasting_address,
                "broadcasting address")["-c"]["--m_broadcasting-address"](
          fmt::format("port to listen (default: {})", m_broadcasting_address)) |
      lyra::opt(m_broadcasting_port,
                "broadcasting port")["-b"]["--broadcasting-port"](
          fmt::format("port to listen (default: {})", m_broadcasting_port)) |
      lyra::opt(m_pool_size, "thread-pool size")["-n"]["--thread-pool-size"](
          fmt::format("The size of a thread pool to run server (default: {})",
                      m_pool_size)) |
      lyra::opt(m_root_dir, "root-dir")["-r"]["--root-dir"](
          fmt::format("server root dir (default: '{}')", m_root_dir)) |
      lyra::opt(m_certs_dir, "certs dir")["--certs-dir"](
          fmt::format("server certs dir (default: '{}')", m_certs_dir)) |
      lyra::opt(m_cors_origin, "cors origin")["--cors-origin"](
          "CORS allowed origin (default: disabled)") |
      lyra::opt(m_api_token, "api token")["--api-token"](
          "Bearer token for API authentication (default: disabled)");

  auto parser_result = cli.parse(lyra::args(argc, argv));
  if (!parser_result) {
    throw std::runtime_error{fmt::format("Invalid command-line arguments: {}",
                                         parser_result.message())};
  }

  if (m_help) {
    SPDLOG_INFO(fmt::streamed(cli));
  }
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
          },
      .udp = {m_udp_port},
      .broadcasting = {m_broadcasting_address, m_broadcasting_port},
      .logger = {20},
      .thread_pool_size = m_pool_size,
  };

  return server_parameters;
}