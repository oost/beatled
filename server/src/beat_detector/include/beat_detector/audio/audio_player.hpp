#ifndef BEAT_TRACKER__AUDIO_PLAYER_HPP
#define BEAT_TRACKER__AUDIO_PLAYER_HPP

#include <filesystem>
#include <string>

#include "config.h"

namespace beatled::detector {

class AudioPlayer {
public:
  AudioPlayer(const std::string &filename, std::size_t audio_buffer_size);
  void play();

private:
  std::filesystem::path absolute_file_path() const;
  void load_from_disk();

  std::string filename_;
  std::vector<audio_buffer_t> audio_data_;
  double sample_rate_;
  std::size_t audio_buffer_size_;
};
} // namespace beatled::detector

#endif // BEAT_TRACKER__AUDIO_PLAYER_HPP