#ifndef BEAT_DETECTOR__AUDIO_OUTPUT_HPP
#define BEAT_DETECTOR__AUDIO_OUTPUT_HPP

#include <beat_tracker.hpp>
#include <filesystem>
#include <portaudio.h>

#include "audio_exception.hpp"
#include "audio_interface.hpp"
#include "beat_detector/audio/config.h"
#include "portaudio_handler.hpp"

namespace fs = std::filesystem;

namespace beatled::detector {

class AudioOutput : public AudioInterface {
public:
  AudioOutput(std::vector<audio_buffer_t> &audio_data,
              AudioBufferPool *audio_buffer_pool, uint32_t sample_rate,
              std::size_t audio_buffer_size,
              unsigned long frames_per_buffer = 0);
  // virtual ~AudioOutput();

private:
  virtual const PaStreamParameters *get_output_parameters();

  /* The instance callback, where we have access to every method/variable in
   * object of class Sine */
  int paCallbackMethod(const void *inputBuffer, void *outputBuffer,
                       unsigned long frameCount,
                       const PaStreamCallbackTimeInfo *timeInfo,
                       PaStreamCallbackFlags statusFlags);

  std::vector<audio_buffer_t> audio_data_;
  int read_index_ = 0;

  PaStreamParameters output_parameters_;
};

} // namespace beatled::detector

#endif // BEAT_DETECTOR__AUDIO_OUTPUT_HPP