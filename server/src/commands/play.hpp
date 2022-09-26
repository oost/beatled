#include <filesystem>
#include <fmt/format.h>
#include <iostream>
#include <lyra/lyra.hpp>
#include <portaudio.h>
#include <string>

#include "../beat_detector/audio_player.hpp"
#include "../beat_detector/portaudio_handler.hpp"
#include "../config.hpp"

namespace fs = std::filesystem;

using namespace beat_detector;

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
      std::cout << g;
    else {
      //---------------------------------------------------------------
      std::cout << "**********************" << std::endl;
      std::cout << "Running Example: Load Audio File and Print Summary"
                << std::endl;
      std::cout << "**********************" << std::endl << std::endl;

      AudioPlayer audio_player_(audioFileName);
      audio_player_.play();
      // ScopedPaHandler paInit;
      // if (paInit.result() != paNoError) {
      //   throw AudioInputException("Couldn't start pa handler.");
      // }

      // beat_detector::AudioOutput audio_output;
      // audio_output.load_from_disk(audioFileName);

      // if (!audio_output.open()) {
      //   throw AudioException("Couldn't open device.");
      // }

      // std::cout << "Output stream opened ! " << std::endl;

      // if (!audio_output.start()) {
      //   throw AudioException("Couldn't start stream.");
      // }

      // std::cout << "Audio output active: " << audio_output.is_active()
      //           << std::endl;

      // while (audio_output.is_active()) {
      //   Pa_Sleep(100);
      // }

      // // Add thread syncronization

      // std::cout << "Audio output active: " << audio_output.is_active()
      //           << std::endl;

      // audio_output.close();
    }
  };
};
