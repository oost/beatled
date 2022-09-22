#include <fmt/format.h>
#include <iostream>
#include <lyra/lyra.hpp>

// struct app_args_t {
//   bool m_help{false};
//   std::string m_root_dir{"."};

//   static app_args_t parse(int argc, const char *argv[]) {

//     app_args_t result;

//     auto cli = lyra::help(result.m_help) |
//                lyra::arg(result.m_root_dir, "root-dir")(fmt::format(
//                    "server root dir (default: '{}')", result.m_root_dir));

//     auto parser_result = cli.parse(lyra::args(argc, argv));
//     if (!parser_result) {
//       throw std::runtime_error{fmt::format("Invalid command-line arguments:
//       {}",
//                                            parser_result.message())};
//     }

//     if (result.m_help) {
//       std::cout << cli << std::endl;
//     }

//     return result;
//   }
// };

// int main(int argc, const char **argv) {
//   std::cout << "Starting audio beat" << std::endl;

//   const auto args = app_args_t::parse(argc, argv);

//   if (!args.m_help) {
//     // std::cout
//   }
//   return 0;
// }

#include <iostream>
#include <string>
#include <vector>

#include "beat_detector/audio_input.hpp"
#include "beat_detector/audio_output.hpp"
#include "beat_detector/portaudio_handler.hpp"
#include "portaudio.h"

#include <iostream>

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
      if (paInit.result() != paNoError)
        return;

      std::cout << "Buffer size: " << BUFFER_SIZE << std::endl;

      AudioInput audio_input(BUFFER_SIZE, sample_rate, frames_per_buffer);

      if (!audio_input.open()) {
        std::cout << "Couldn't open device." << std::endl;
        return;
      }

      if (!audio_input.start()) {
        std::cout << "Couldn't start stream." << std::endl;
        return;
      }

      std::cout << "Audio input active: " << audio_input.is_active()
                << std::endl;
      while (audio_input.is_active()) {
        Pa_Sleep(1000);
      }
      // Add thread syncronization

      std::cout << "Audio input active: " << audio_input.is_active()
                << std::endl;

      audio_input.save_to_disk(audioFileName);

      std::cout << "Saved audio to " << audioFileName << std::endl;
    }
  };
};

int main(int argc, const char **argv) {
  auto cli = lyra::cli();
  std::string command;
  bool show_help = false;
  cli.add_argument(lyra::help(show_help));

  record_audio_command record{cli};

  auto result = cli.parse({argc, argv});
  if (show_help) {
    std::cout << cli;
    return 0;
  }

  if (!result) {
    std::cerr << result.message() << "\n";
  }

  return result ? 0 : 1;
}