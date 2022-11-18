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
#include "config/config.hpp"
#include "server/server.hpp"
#include "state_manager/state_manager.hpp"

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

  const auto beatled_config = BeatledConfig(argc, argv);

  if (!beatled_config.help()) {

    // // Initialize our singleton in the main thread
    StateManager state_manager;

    // // Let's start the beat detector thread.
    asio::thread bd_thread([&state_manager]() {
      beat_detector::BeatDetector bd(state_manager, 44100);
      bd.run();
    });

    server::server_parameters_t server_parameters =
        beatled_config.server_parameters();

    server::Server server(state_manager, server_parameters);
    server.run();

    bd_thread.join();
  }
  // } catch (const std::exception &ex) {
  //   std::cerr << "Error: " << ex.what() << std::endl;
  //   return 1;
  // }

  return 0;
}
