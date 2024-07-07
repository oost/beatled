#include <fmt/format.h>
#include <fmt/ostream.h>
#include <iostream>
#include <lyra/lyra.hpp>

#include "commands/play.hpp"
#include "commands/record.hpp"
#include "commands/track.hpp"
#include "commands/track_next_beat.hpp"

int main(int argc, const char **argv) {
  auto cli = lyra::cli();
  bool show_help = false;
  cli.add_argument(lyra::help(show_help));

  record_audio_command record{cli};
  play_audio_command play{cli};
  track_beat_command track{cli};
  track_next_beat_command track_next_beat{cli};

  try {
    auto result = cli.parse({argc, argv});
    if (show_help) {
      SPDLOG_INFO(fmt::streamed(cli));
      return 0;
    }

    if (!result) {
      std::cerr << result.message() << std::endl;
    }
    return result ? 0 : 1;

  } catch (const std::exception &exception) {
    std::cerr << "An exception occured (" << exception.what() << ")"
              << std::endl;
  }
  return 1;
}