#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/std.h>
#include <spdlog/spdlog.h>

#include "audio_buffer_pool.hpp"
#include "audio_input.hpp"
#include "beat_detector/audio/audio_recorder.hpp"

namespace beatled::detector {

AudioRecorder::AudioRecorder(const std::string &filename, double duration,
                             double sample_rate, double frames_per_buffer,
                             std::size_t audio_buffer_size)
    : filename_{filename}, duration_{duration}, sample_rate_{sample_rate},
      audio_buffer_size_{audio_buffer_size} {}

std::string AudioRecorder::record() {
  const unsigned long TOTAL_BUFFER_SIZE = sample_rate_ * duration_;

  AudioBufferPool audio_buffer_pool{audio_buffer_size_, sample_rate_};

  // Use frame_rate = 0 to let the OS choose the frame rate (potentially
  // dynamcially)
  AudioInput audio_input(&audio_buffer_pool, sample_rate_);
  // AudioInput audio_input(audio_buffer_pool, sample_rate_,
  // frames_per_buffer_);

  if (!audio_input.open()) {
    throw AudioInputException("Couldn't open device.");
  }

  if (!audio_input.start()) {
    throw AudioInputException("Couldn't start stream.");
  }

  SPDLOG_INFO("Audio input active: {}", audio_input.is_active());

  AudioFile<audio_buffer_t> audio_file;
  audio_file.samples.resize(1);
  auto &audio_data = audio_file.samples[0];
  audio_data.reserve(TOTAL_BUFFER_SIZE);
  size_t audio_data_remaining_capacity = audio_data.capacity();
  int idx = 0;
  const int cycle_per_second =
      static_cast<int>(sample_rate_) / audio_buffer_size_;

  AudioBuffer::Ptr buffer;
  while (1) {
    buffer = audio_buffer_pool.dequeue_blocking();
    if (!buffer) {
      SPDLOG_INFO("Stopping thread");
      audio_input.stop();
      break;
    }

    size_t elements_to_copy = (buffer->size() > audio_data_remaining_capacity)
                                  ? audio_data_remaining_capacity
                                  : buffer->size();

    const AudioBuffer::audio_buffer_data_t &buffer_data = buffer->data();
    for (size_t i = 0; i < elements_to_copy; i++) {
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
      SPDLOG_INFO("Recorded {} seconds", idx * audio_buffer_size_ / sample_rate_);
    }
  }

  fs::path audio_file_path = absolute_file_path();

  SPDLOG_INFO("Saving audio file to: {}", audio_file_path);
  // Wave file (explicit)
  if (!audio_file.save(audio_file_path, AudioFileFormat::Wave)) {
    throw AudioFileException("Error saving file.");
  }

  SPDLOG_INFO("Audio input active: {}", audio_input.is_active());

  SPDLOG_INFO("Saved audio to {}", audio_file_path);
  return audio_file_path;
}

std::filesystem::path AudioRecorder::absolute_file_path() const {
  fs::path audio_file_path = filename_;
  if (audio_file_path.is_relative()) {
    audio_file_path = fs::current_path() / audio_file_path;
  }
  return audio_file_path;
}

} // namespace beatled::detector