#ifndef BEAT_TRACKER__AUDIO_PLAYER_HPP
#define BEAT_TRACKER__AUDIO_PLAYER_HPP

#include <string>

namespace beat_detector {

class AudioPlayer {
public:
  AudioPlayer(const std::string &filename);
  void play();

private:
  std::string filename_;
};
} // namespace beat_detector

#endif // BEAT_TRACKER__AUDIO_PLAYER_HPP