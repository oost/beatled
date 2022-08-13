#include <fmt/format.h>
#include <iostream>
#include <map>
#include <thread>
#include <asio.hpp>
#include <lyra/lyra.hpp>

#include "beat/beat_worker.h"
#include "build_constants.h"
#include "http/server_handler.h"

struct app_args_t
{
  bool m_help{false};
  std::string m_address{"localhost"};
  std::uint16_t m_port{8080};
  std::size_t m_pool_size{1};
  std::string m_root_dir{"."};

  static app_args_t parse(int argc, const char *argv[])
  {

    app_args_t result;

    auto cli =
        lyra::help(result.m_help) |
        lyra::opt(result.m_address, "address")["-a"]["--address"](
            fmt::format("address to listen (default: {})", result.m_address)) |
        lyra::opt(result.m_port, "port")["-p"]["--port"](
            fmt::format("port to listen (default: {})", result.m_port)) |
        lyra::opt(result.m_pool_size, "thread-pool size")["-n"]["--thread-pool-size"](
            fmt::format("The size of a thread pool to run server (default: {})",
                        result.m_pool_size)) |
        lyra::arg(result.m_root_dir, "root-dir")(
            fmt::format("server root dir (default: '{}')", result.m_root_dir));

    auto parser_result = cli.parse(lyra::args(argc, argv));
    if (!parser_result)
    {
      throw std::runtime_error{fmt::format("Invalid command-line arguments: {}",
                                           parser_result.message())};
    }

    if (result.m_help)
    {
      std::cout << cli << std::endl;
    }

    return result;
  }
};

void print_version(const char *command)
{
  std::cout << command << " Version " << RPIZ_BS_VERSION_MAJOR << "."
            << RPIZ_BS_VERSION_MINOR << std::endl
            << "Compiled on " << RPIZ_BS_BUILDTIME << std::endl;
}

int main(int argc, char const *argv[])
{

  try
  {

    print_version(argv[0]);

    const auto args = app_args_t::parse(argc, argv);

    if (!args.m_help)
    {
      asio::io_context io_context;
      std::thread beat_worker_thread(start_beat_worker);

      start_http_server(args.m_pool_size, args.m_address, args.m_port,
                        args.m_root_dir);

      beat_worker_thread.join();
    }
  }
  catch (const std::exception &ex)
  {
    std::cerr << "Error: " << ex.what() << std::endl;
    return 1;
  }

  return 0;
}
