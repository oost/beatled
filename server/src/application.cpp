#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include "./application.hpp"

const char *BEAT_DETECTOR_ID = "beat-detector";
const char *HTTP_SERVER_ID = "http-server";
const char *UDP_SERVER_ID = "udp-server";
const char *TEMPO_BROADCASTER_ID = "tempo-broadcaster";

BeatledApplication::BeatledApplication(const BeatledConfig &beatled_config)
    : server_parameters_{beatled_config.server_parameters()},
      logger_{server_parameters_.logger}, beat_detector_{BEAT_DETECTOR_ID,
                                                         state_manager_, 44100},
      server_{server_parameters_}, udp_server_{UDP_SERVER_ID,
                                               server_.io_context(),
                                               server_parameters_.udp,
                                               state_manager_},
      http_server_{HTTP_SERVER_ID, server_parameters_.http, *this,
                   server_.io_context(), logger_},
      tempo_broadcaster_{TEMPO_BROADCASTER_ID,
                         server_.io_context(),
                         std::chrono::milliseconds((60 * 1000) / 120),
                         std::chrono::seconds(2),
                         server_parameters_.broadcasting,
                         state_manager_} {
  registerController(beat_detector_);
  registerController(udp_server_);
  registerController(http_server_);
  registerController(tempo_broadcaster_);
}

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
