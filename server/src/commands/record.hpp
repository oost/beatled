#include <fmt/format.h>
#include <iostream>
#include <lyra/lyra.hpp>
#include <portaudio.h>
#include <string>

#include "../beat_detector/audio_input.hpp"
#include "../beat_detector/audio_output.hpp"
#include "../beat_detector/audio_recorder.hpp"
#include "../beat_detector/portaudio_handler.hpp"

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
            .add_argument(
                lyra::opt(sample_rate, "sample_rate")
                    .name("-s")
                    .name("--sample-rate")
                    .help(fmt::format("Sample at which rate? (default: {})",
                                      sample_rate)))
            .add_argument(lyra::opt(frames_per_buffer, "frames_per_buffer")
                              .name("-f")
                              .name("--frames-per-buffer")
                              .help(fmt::format(
                                  "How many frames per buffer? (default: {})",
                                  frames_per_buffer)))
            .add_argument(lyra::opt(duration, "duration")
                              .name("-d")
                              .name("--duration")
                              .help(fmt::format("Which duration? (default: {})",
                                                duration)))
            .add_argument(
                lyra::opt(audioFileName, "audio file name")
                    .name("-o")
                    .name("--output")
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
      SPDLOG_INFO(g);
    else {

      using namespace beat_detector;

      AudioRecorder recorder{audioFileName, duration, sample_rate,
                             static_cast<double>(frames_per_buffer)};

      std::string audio_file_path = recorder.record();

      SPDLOG_INFO("Saved audio to {}", audio_file_path);
    }
  };
};
