#include <fmt/format.h>
#include <iostream>
#include <lyra/lyra.hpp>
#include <map>
#include <memory>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <thread>

#include "beat_detector/beat_detector.hpp"
#include "build_constants.h"
#include "server/server.hpp"
#include "state_manager/state_manager.hpp"

struct app_args_t {
  bool m_help{false};
  std::string m_address{"localhost"};
  bool m_start_http_server = false;
  bool m_start_udp_server = false;
  bool m_start_broadcaster = false;
  std::uint16_t m_http_port{8080};
  std::uint16_t m_udp_port{9090};
  std::string m_broadcasting_address{"192.168.86.255"};
  std::uint16_t m_broadcasting_port{8765};
  std::size_t m_pool_size{2};
  std::string m_root_dir{"."};

  static app_args_t parse(int argc, const char *argv[]) {

    app_args_t result;

    auto cli =
        lyra::help(result.m_help) |
        lyra::opt(result.m_address, "address")["-a"]["--address"](
            fmt::format("address to listen (default: {})", result.m_address)) |
        lyra::opt(result.m_start_http_server)["--start-http"](
            "Start HTTP server") |
        lyra::opt(result.m_start_udp_server)["--start-udp"](
            "Start UDP server") |
        lyra::opt(result.m_start_broadcaster)["--start-http"](
            "Start broadcaster ") |

        lyra::opt(result.m_http_port, "http port")["-p"]["--http-port"](
            fmt::format("port to listen (default: {})", result.m_http_port)) |
        lyra::opt(result.m_udp_port, "udp port")["-u"]["--udp-port"](
            fmt::format("port to listen (default: {})", result.m_udp_port)) |
        lyra::opt(result.m_broadcasting_address,
                  "broadcasting address")["-c"]["--m_broadcasting-address"](
            fmt::format("port to listen (default: {})",
                        result.m_broadcasting_address)) |
        lyra::opt(result.m_broadcasting_port,
                  "broadcasting port")["-b"]["--broadcasting-port"](fmt::format(
            "port to listen (default: {})", result.m_broadcasting_port)) |
        lyra::opt(result.m_pool_size,
                  "thread-pool size")["-n"]["--thread-pool-size"](
            fmt::format("The size of a thread pool to run server (default: {})",
                        result.m_pool_size)) |
        lyra::arg(result.m_root_dir, "root-dir")(
            fmt::format("server root dir (default: '{}')", result.m_root_dir));

    auto parser_result = cli.parse(lyra::args(argc, argv));
    if (!parser_result) {
      throw std::runtime_error{fmt::format("Invalid command-line arguments: {}",
                                           parser_result.message())};
    }

    if (result.m_help) {
      std::cout << cli << std::endl;
    }

    return result;
  }
};

void print_version(const char *command) {
  std::cout << command << " Version " << RPIZ_BS_VERSION_MAJOR << "."
            << RPIZ_BS_VERSION_MINOR << std::endl
            << "Compiled on " << RPIZ_BS_BUILDTIME << std::endl;
}

int main(int argc, char const *argv[]) {

  // try {
  // // create color multi threaded logger
  auto console = spdlog::stdout_color_mt("console");
  auto err_logger = spdlog::stderr_color_mt("stderr");
  spdlog::get("console")->info("Starting beat log ! ");
  spdlog::flush_every(std::chrono::seconds(1));

  print_version(argv[0]);

  const auto args = app_args_t::parse(argc, argv);

  if (!args.m_help) {

    // // Initialize our singleton in the main thread
    // StateManager::initialize();

    // // Let's start the beat detector thread.
    // asio::thread bd_thread([]() {
    //   BeatDetector bd;
    //   bd.run();
    // });

    server::server_parameters_t server_parameters{
        .start_http_server = args.m_start_http_server,
        .start_udp_server = args.m_start_udp_server,
        .start_broadcaster = args.m_start_broadcaster,
        .http =
            {
                args.m_address,   // address
                args.m_http_port, // port
                args.m_root_dir   // root_dir
            },
        .udp = {args.m_udp_port},
        .broadcasting = {args.m_broadcasting_address, args.m_broadcasting_port},
        .logger = {20},
        .thread_pool_size = args.m_pool_size,
    };

    server::Server server(server_parameters);
    server.run();

    // bd_thread.join();
  }
  // } catch (const std::exception &ex) {
  //   std::cerr << "Error: " << ex.what() << std::endl;
  //   return 1;
  // }

  return 0;
}
