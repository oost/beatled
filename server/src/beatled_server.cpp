#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include "./application.hpp"
#include "./build_constants.h"

void print_version(const char *command) {
  SPDLOG_INFO("{} Version {}.{}. Compiled on {}", command,
              RPIZ_BS_VERSION_MAJOR, RPIZ_BS_VERSION_MINOR, RPIZ_BS_BUILDTIME);
}

int main(int argc, char const *argv[]) {
  struct server::logger_parameters_t logger_parameters = {.queue_size = 20};
  auto logger = server::Logger(logger_parameters);
  auto console = spdlog::stdout_color_mt("console");
  auto err_logger = spdlog::stderr_color_mt("stderr");
  SPDLOG_INFO("Starting log ");

  try {
    print_version(argv[0]);

    const auto beatled_config = BeatledConfig(argc, argv);

    if (!beatled_config.help()) {
      BeatledApplication app(beatled_config);

      app.start();
    }
  } catch (const std::exception &ex) {
    SPDLOG_ERROR("Caught exception: {}", ex.what());

    return 1;
  }

  return 0;
}
