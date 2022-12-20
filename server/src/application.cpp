#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include "application.hpp"

BeatledApplication::BeatledApplication(const BeatledConfig &beatled_config)
    : beatled_config_{beatled_config}, beat_detector_{state_manager_, 44100} {
  server_ = std::make_unique<server::Server>(
      state_manager_, beat_detector_, beatled_config_.server_parameters());
}

void BeatledApplication::start() {

  // start_beat_detector();
  server_->run();
  SPDLOG_INFO("Stopped servers. Waiting for beat detection thread.");
  beat_detector_.stop_blocking();
}

void BeatledApplication::start_beat_detector() { beat_detector_.run(); }

void BeatledApplication::stop_beat_detector() { beat_detector_.request_stop(); }