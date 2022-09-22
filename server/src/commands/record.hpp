#include <fmt/format.h>
#include <iostream>
#include <lyra/lyra.hpp>
#include <portaudio.h>
#include <string>

#include "../beat_detector/audio_input.hpp"
#include "../beat_detector/audio_output.hpp"
#include "../beat_detector/portaudio_handler.hpp"

using namespace beat_detector;

/*******************************************************************/
struct record_audio_command {
  bool verbose = false;
  bool show_help = false;
  double sample_rate = 44100;
  unsigned long frames_per_buffer = 1024;
  double duration = 2;
  std::string audioFileName = "audio_file.wav";

  record_audio_command(lyra::cli &cli) {
    cli.add_argument(

        lyra::command("record",
                      [this](const lyra::group &g) { this->do_command(g); })
            .help("Record audio.")
            .add_argument(lyra::help(show_help))
            .add_argument(lyra::opt(sample_rate, "sample_rate")
                              .name("-s")
                              .name("--sample-rate")
                              .help("Sample at which rate?"))
            .add_argument(lyra::opt(frames_per_buffer, "frames_per_buffer")
                              .name("-f")
                              .name("--frames-per-buffer")
                              .help("How many frames per buffer?"))
            .add_argument(lyra::opt(duration, "duration")
                              .name("-d")
                              .name("--duration")
                              .help("Which duration?"))
            .add_argument(lyra::opt(audioFileName, "audio file name")
                              .name("-o")
                              .name("--output")
                              .help("Where do we save the audio"))
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
      const unsigned long BUFFER_SIZE = sample_rate * duration;

      ScopedPaHandler paInit;
      if (paInit.result() != paNoError) {
        throw AudioInputException("Couldn't start pa handler.");
      }

      std::cout << "Buffer size: " << BUFFER_SIZE << std::endl;

      AudioInput audio_input(BUFFER_SIZE, sample_rate, frames_per_buffer);

      if (!audio_input.open()) {
        throw AudioInputException("Couldn't open device.");
      }

      if (!audio_input.start()) {
        throw AudioInputException("Couldn't start stream.");
      }

      std::cout << "Audio input active: " << audio_input.is_active()
                << std::endl;

      while (audio_input.is_active()) {
        Pa_Sleep(1000);
      }
      // Add thread syncronization

      std::cout << "Audio input active: " << audio_input.is_active()
                << std::endl;

      std::string audioFilePath = audio_input.save_to_disk(audioFileName);

      std::cout << "Saved audio to " << audioFilePath << std::endl;
    }
  };
};
