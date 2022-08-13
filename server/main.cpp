#include <clara.hpp>
#include <fmt/format.h>
#include <iostream>
#include <map>
#include <thread>

#include "beat/beat_worker.h"
#include "build_constants.h"
#include "http/server_handler.h"

struct app_args_t {
  bool m_help{false};
  std::string m_address{"localhost"};
  std::uint16_t m_port{8080};
  std::size_t m_pool_size{1};
  std::string m_root_dir{"."};

  static app_args_t parse(int argc, const char *argv[]) {
    using namespace clara;

    app_args_t result;

    auto cli =
        Opt(result.m_address, "address")["-a"]["--address"](
            fmt::format("address to listen (default: {})", result.m_address)) |
        Opt(result.m_port, "port")["-p"]["--port"](
            fmt::format("port to listen (default: {})", result.m_port)) |
        Opt(result.m_pool_size, "thread-pool size")["-n"]["--thread-pool-size"](
            fmt::format("The size of a thread pool to run server (default: {})",
                        result.m_pool_size)) |
        Arg(result.m_root_dir, "root-dir")(
            fmt::format("server root dir (default: '{}')", result.m_root_dir)) |
        Help(result.m_help);

    auto parse_result = cli.parse(Args(argc, argv));
    if (!parse_result) {
      throw std::runtime_error{fmt::format("Invalid command-line arguments: {}",
                                           parse_result.errorMessage())};
    }

    if (result.m_help) {
      std::cout << cli << std::endl;
    }

    return result;
  }
};

int main(int argc, char const *argv[]) {

  try {
    std::cout << argv[0] << " Version " << RPIZ_BS_VERSION_MAJOR << "."
              << RPIZ_BS_VERSION_MINOR << std::endl
              << "Compiled on " << RPIZ_BS_BUILDTIME << std::endl;

    const auto args = app_args_t::parse(argc, argv);

    if (!args.m_help) {
      std::thread beat_worker_thread(start_beat_worker);

      start_http_server(args.m_pool_size, args.m_address, args.m_port,
                        args.m_root_dir);

      beat_worker_thread.join();
    }
  } catch (const std::exception &ex) {
    std::cerr << "Error: " << ex.what() << std::endl;
    return 1;
  }

  return 0;
}
