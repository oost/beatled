#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include "./application.hpp"
#include "./build_constants.h"
#include "core/realtime.hpp"

using beatled::core::Config;

void print_version(const char *command) {
  SPDLOG_INFO("{} Version {}.{}. Compiled on {}", command, RPIZ_BS_VERSION_MAJOR,
              RPIZ_BS_VERSION_MINOR, RPIZ_BS_BUILDTIME);
}

int main(int argc, char const *argv[]) {
  struct beatled::server::Logger::parameters_t logger_parameters = {.queue_size = 20};
  auto logger = beatled::server::Logger(logger_parameters);
  auto console = spdlog::stdout_color_mt("console");
  auto err_logger = spdlog::stderr_color_mt("stderr");
  SPDLOG_INFO("Starting log ");

  try {
    print_version(argv[0]);

    // Pin memory up front so neither the network worker pool nor the
    // beat-detector loop ever stalls on a major page fault. No-op off Linux;
    // a soft failure if the process lacks the memlock privilege.
    beatled::core::lock_process_memory();

    const auto beatled_config = Config(argc, argv);
    beatled_config.log_config();

    if (!beatled_config.help()) {
      beatled::Application app(beatled_config);

      app.run();
    }
  } catch (const std::exception &ex) {
    SPDLOG_ERROR("Caught exception: {}", ex.what());

    return 1;
  }

  return 0;
}
