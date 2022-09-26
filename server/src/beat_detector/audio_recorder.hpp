#ifndef BEAT_TRACKER__AUDIO_RECORDER_HPP
#define BEAT_TRACKER__AUDIO_RECORDER_HPP

#include <filesystem>
#include <string>

namespace beat_detector {

class AudioRecorder {
public:
  AudioRecorder(const std::string &filename, double duration,
                double sample_rate, double frames_per_buffer);
  std::string record();

private:
  std::filesystem::path absolute_file_path() const;

  std::string filename_;
  double duration_;
  double sample_rate_;
  double frames_per_buffer_;
};
} // namespace beat_detector

#endif // BEAT_TRACKER__AUDIO_RECORDER_HPP