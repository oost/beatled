#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include "./application.hpp"

BeatledApplication::BeatledApplication(const BeatledConfig &beatled_config)
    : server_parameters_{beatled_config.server_parameters()},
      logger_{server_parameters_.logger},
      beat_detector_{state_manager_, 44100}, server_{server_parameters_},
      udp_server_{server_.io_context(), server_parameters_.udp, state_manager_},
      http_server_{server_parameters_.http, *this, server_.io_context(),
                   logger_},
      tempo_broadcaster_{server_.io_context(),
                         std::chrono::milliseconds((60 * 1000) / 120),
                         std::chrono::seconds(2),
                         server_parameters_.broadcasting, state_manager_} {}

void BeatledApplication::run() {
  // start_beat_detector();
  if (server_parameters_.start_udp_server) {
    udp_server_.start();
  }

  if (server_parameters_.start_broadcaster) {
    tempo_broadcaster_.start();
  }

  if (server_parameters_.start_http_server) {
    http_server_.start();
  }

  server_.run();
  SPDLOG_INFO("Stopped servers. Waiting for beat detection thread.");
  beat_detector_.stop_blocking();
}
