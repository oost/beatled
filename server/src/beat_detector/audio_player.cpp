#include "fmt/format.h"

#include "audio_buffer_pool.hpp"
#include "audio_output.hpp"
#include "audio_player.hpp"

using namespace beat_detector;

AudioPlayer::AudioPlayer(const std::string &filename) : filename_{filename} {}

void AudioPlayer::play() {
  fs::path audio_file_path = filename_;
  if (audio_file_path.is_relative()) {
    audio_file_path = fs::current_path() / audio_file_path;
  }
  AudioOutput audio_output;
  audio_output.load_from_disk(audio_file_path);

  if (!audio_output.open()) {
    throw AudioException("Couldn't open device.");
  }

  std::cout << "Output stream opened ! " << std::endl;

  if (!audio_output.start()) {
    throw AudioException("Couldn't start stream.");
  }

  std::cout << "Audio output active: " << audio_output.is_active() << std::endl;

  while (audio_output.is_active()) {
    Pa_Sleep(100);
  }

  // Add thread syncronization

  std::cout << "Audio output active: " << audio_output.is_active() << std::endl;

  audio_output.close();
}