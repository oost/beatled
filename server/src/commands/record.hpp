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
      std::cout << g;
    else {
      const unsigned long TOTAL_BUFFER_SIZE = sample_rate * duration;

      ScopedPaHandler paInit;
      if (paInit.result() != paNoError) {
        throw AudioInputException("Couldn't start pa handler.");
      }

      std::cout << "Total buffer size: " << TOTAL_BUFFER_SIZE << std::endl;

      AudioBufferPool audio_buffer_pool{constants::audio_buffer_size};

      AudioInput audio_input(audio_buffer_pool, sample_rate, frames_per_buffer);

      if (!audio_input.open()) {
        throw AudioInputException("Couldn't open device.");
      }

      if (!audio_input.start()) {
        throw AudioInputException("Couldn't start stream.");
      }

      std::cout << "Audio input active: " << audio_input.is_active()
                << std::endl;

      AudioFile<audio_buffer_t> audio_file;
      audio_file.samples.resize(1);
      auto &audio_data = audio_file.samples[0];
      audio_data.reserve(TOTAL_BUFFER_SIZE);
      float sampleRate = 44100.f;
      float frequency = 440.f;

      int audio_data_remaining_capacity = audio_data.capacity();
      int idx = 0;
      const int cycle_per_second =
          static_cast<int>(sampleRate) / constants::audio_buffer_size;
      while (1) {
        AudioBuffer_ptr buffer = audio_buffer_pool.dequeue_blocking();
        int elements_to_copy = (buffer->size() > audio_data_remaining_capacity)
                                   ? audio_data_remaining_capacity
                                   : buffer->size();

        const audio_buffer_data_t &buffer_data = buffer->data();
        for (int i = 0; i < elements_to_copy; i++) {
          audio_data.push_back(buffer_data[i]);
        }
        audio_buffer_pool.release_buffer(std::move(buffer));
        audio_data_remaining_capacity -= elements_to_copy;
        if (audio_data_remaining_capacity == 0) {
          audio_input.stop();
          break;
        }

        idx++;
        if (idx % cycle_per_second == 0) {
          std::cout << fmt::format("Recorded {} seconds",
                                   idx * constants::audio_buffer_size /
                                       sampleRate)
                    << std::endl;
        }
      }

      fs::path audio_file_path = audioFileName;
      if (audio_file_path.is_relative()) {
        audio_file_path = fs::current_path() / audio_file_path;
      }

      std::cout << "Saving audio file to: " << audio_file_path << std::endl;
      // Wave file (explicit)
      if (!audio_file.save(audio_file_path, AudioFileFormat::Wave)) {
        throw AudioFileException("Error saving file.");
      }

      std::cout << "Audio input active: " << audio_input.is_active()
                << std::endl;

      std::cout << "Saved audio to " << audio_file_path << std::endl;
    }
  };
};
