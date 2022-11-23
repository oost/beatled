#include <fmt/format.h>
#include <fmt/ostream.h>
#include <iostream>
#include <lyra/lyra.hpp>

#include "commands/play.hpp"
#include "commands/record.hpp"
#include "commands/track.hpp"

int main(int argc, const char **argv) {
  auto cli = lyra::cli();
  bool show_help = false;
  cli.add_argument(lyra::help(show_help));

  record_audio_command record{cli};
  play_audio_command play{cli};
  track_beat_command track{cli};

  try {
    auto result = cli.parse({argc, argv});
    if (show_help) {
      SPDLOG_INFO(fmt::streamed(cli));
      return 0;
    }

    if (!result) {
      std::cerr << result.message() << "\n";
    }
    return result ? 0 : 1;

  } catch (const std::exception &exception) {
    std::cerr << "An exception occured (" << exception.what() << ")\n";
  }
  return 1;
}