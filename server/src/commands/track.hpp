#include <fmt/format.h>
#include <iostream>
#include <lyra/lyra.hpp>
#include <string>

#include "../beat_detector/beat_detector.hpp"

/*******************************************************************/
struct track_beat_command {
  bool verbose = false;
  bool show_help = false;
  double duration = 10;

  track_beat_command(lyra::cli &cli) {
    cli.add_argument(

        lyra::command("track",
                      [this](const lyra::group &g) { this->do_command(g); })
            .help("Track audio")
            .add_argument(lyra::help(show_help))
            .add_argument(lyra::opt(duration, "duration")
                              .name("-d")
                              .name("--duration")
                              .help(fmt::format("Which duration? (default: {})",
                                                duration)))
            .add_argument(
                lyra::opt(verbose)
                    .name("-v")
                    .name("--verbose")
                    .optional()
                    .help("Show additional output as to what we are doing.")));
  }
  void do_command(const lyra::group &g) {
    if (show_help)
      std::cout << g;
    else {
      using namespace beat_detector;

      using namespace std::chrono_literals;

      BeatDetector bd{44100};

      bd.run();
      std::cout << "Started Beat Detector" << std::endl;
      std::this_thread::sleep_for(
          std::chrono::seconds(static_cast<int>(duration)));
      bd.request_stop();

      // std::this_thread::sleep_for(2000ms);
      // bd.run();
      // bd.run();
      // std::this_thread::sleep_for(2000ms);
      // bd.request_stop();
    }
  };
};
