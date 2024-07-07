#include <filesystem>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <iostream>
#include <lyra/lyra.hpp>
#include <spdlog/spdlog.h>
#include <string>

#include "../config.hpp"
#include "beat_detector/audio/audio_player.hpp"
#include "beat_detector/audio/config.h"

/*******************************************************************/
struct play_audio_command {
  bool verbose = false;
  bool show_help = false;
  std::string audioFileName = "audio_file.wav";

  play_audio_command(lyra::cli &cli) {
    cli.add_argument(

        lyra::command("play",
                      [this](const lyra::group &g) { this->do_command(g); })
            .help("Play audio.")
            .add_argument(lyra::help(show_help))
            .add_argument(
                lyra::arg(audioFileName, "audio_file_name")
                    .required()
                    .help(fmt::format("Where do we save the audio(default: {})",
                                      audioFileName)))
            .add_argument(
                lyra::opt(verbose)
                    .name("-v")
                    .name("--verbose")
                    .optional()
                    .help("Show additional output as to what we are doing.")));
  }
  void do_command(const lyra::group &g) {
    if (show_help)
      SPDLOG_INFO(fmt::streamed(g));
    else {
      namespace fs = std::filesystem;

      using namespace beatled::detector;
      //---------------------------------------------------------------
      SPDLOG_INFO("**********************");
      SPDLOG_INFO("Running Example: Load Audio File and Print Summary");
      SPDLOG_INFO("**********************");

      AudioPlayer audio_player_(audioFileName,
                                beatled::constants::audio_buffer_size);
      audio_player_.play();
    }
  };
};
