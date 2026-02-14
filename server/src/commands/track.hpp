#include <fmt/format.h>
#include <fmt/ostream.h>
#include <iostream>
#include <lyra/lyra.hpp>
#include <string>

#include "../config.hpp"
#include "beat_detector/beat_detector.hpp"
#include "core/state_manager.hpp"

using beatled::core::StateManager;

/*******************************************************************/
struct track_beat_command {
  bool verbose = false;
  bool show_help = false;
  double duration = 10;

  track_beat_command(lyra::cli &cli) {
    cli.add_argument(

        lyra::command("track", [this](const lyra::group &g) { this->do_command(g); })
            .help("Track audio")
            .add_argument(lyra::help(show_help))
            .add_argument(lyra::opt(duration, "duration")
                              .name("-d")
                              .name("--duration")
                              .help(fmt::format("Which duration? (default: {})", duration)))
            .add_argument(lyra::opt(verbose)
                              .name("-v")
                              .name("--verbose")
                              .optional()
                              .help("Show additional output as to what we are doing.")));
  }
  void do_command(const lyra::group &g) {
    if (show_help)
      SPDLOG_INFO(fmt::streamed(g));
    else {
      using namespace beatled::detector;

      using namespace std::chrono_literals;
      constexpr int sample_rate = 41000;
      StateManager state_manager;

      BeatDetector bd{
          "beat-detector", sample_rate, beatled::constants::audio_buffer_size,
          [&](uint64_t delay, double tempo, double estimated_tempo, uint32_t beat_count) {
            SPDLOG_INFO("Beat ... tempo {} ... estimated {} ... beat count {}", tempo,
                        estimated_tempo, beat_count);
          }};

      bd.start();
      SPDLOG_INFO("Started Beat Detector");
      std::this_thread::sleep_for(std::chrono::seconds(static_cast<int>(duration)));
      SPDLOG_INFO(" Requesting stop");
      bd.stop();
      SPDLOG_INFO(" BeatDetector stopped");
    }
  };
};
