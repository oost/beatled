#ifndef BEAT_TRACKER__AUDIO_PLAYER_HPP
#define BEAT_TRACKER__AUDIO_PLAYER_HPP

#include <string>

#include "../config.hpp"

namespace beat_detector {

class AudioPlayer {
public:
  AudioPlayer(const std::string &filename);
  void play();

private:
  std::filesystem::path absolute_file_path() const;
  void load_from_disk();

  std::string filename_;
  std::vector<audio_buffer_t> audio_data_;
  uint32_t sample_rate_;
};
} // namespace beat_detector

#endif // BEAT_TRACKER__AUDIO_PLAYER_HPP