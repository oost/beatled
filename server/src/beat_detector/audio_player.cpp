#include "audiofile.h"
#include <fmt/ostream.h>
#include <spdlog/spdlog.h>

#include "audio_buffer_pool.hpp"
#include "audio_output.hpp"
#include "audio_player.hpp"

using namespace beat_detector;

AudioPlayer::AudioPlayer(const std::string &filename) : filename_{filename} {}

void AudioPlayer::play() {
  load_from_disk();

  AudioOutput audio_output(audio_data_, sample_rate_);

  if (!audio_output.open()) {
    throw AudioException("Couldn't open device.");
  }

  SPDLOG_INFO("Output stream opened ! ");

  if (!audio_output.start()) {
    throw AudioException("Couldn't start stream.");
  }

  SPDLOG_INFO("Audio output active: {}", audio_output.is_active());

  while (audio_output.is_active()) {
    Pa_Sleep(100);
  }

  // Add thread syncronization

  SPDLOG_INFO("Audio output active: {}", audio_output.is_active());

  audio_output.close();
}

void AudioPlayer::load_from_disk() {
  //---------------------------------------------------------------
  // 1. Set a file path to an audio file on your machine
  auto filePath = absolute_file_path();

  //---------------------------------------------------------------
  // 2. Create an AudioFile object and load the audio file

  AudioFile<audio_buffer_t> audio_file;
  bool loadedOK = audio_file.load(filePath);

  /** If you hit this assert then the file path above
   probably doesn't refer to a valid audio file */
  assert(loadedOK);

  sample_rate_ = audio_file.getSampleRate();

  // or, just use this quick shortcut to print a summary to the console
  audio_file.printSummary();

  // if (audio_file.getNumChannels() > 1) {
  if (!audio_file.isMono()) {
    throw AudioException("Multiple channels not supported (yet)...");
  }

  int channel = 0;

  audio_data_.resize(audio_file.getNumSamplesPerChannel());
  for (int i = 0; i < audio_file.getNumSamplesPerChannel(); i++) {
    audio_data_[i] = audio_file.samples[channel][i];
  }
}

std::filesystem::path AudioPlayer::absolute_file_path() const {
  fs::path audio_file_path = filename_;
  if (audio_file_path.is_relative()) {
    audio_file_path = fs::current_path() / audio_file_path;
  }
  if (!is_regular_file(audio_file_path)) {
    throw new AudioException("File doesn't exist");
  }
  return audio_file_path;
}