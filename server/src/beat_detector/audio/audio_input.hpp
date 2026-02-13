#ifndef BEATDETECTOR_AUDIOINPUT_H
#define BEATDETECTOR_AUDIOINPUT_H

#include <AudioFile/AudioFile.h>
#include <exception>
#include <filesystem>
#include <iostream>
#include <math.h>
#include <portaudio.h>
#include <stdio.h>
#include <vector>

#include "audio_buffer_pool.hpp"
#include "audio_exception.hpp"
#include "audio_interface.hpp"
#include "beat_detector/audio/config.h"
#include "portaudio_handler.hpp"

namespace beatled::detector {

namespace fs = std::filesystem;

/**
 * @brief Wrapper around an audio input (using PortAudio)
 *
 * Wrapper around PortAudio to allow for audio inputs.
 * Data is captured into `AudioBuffer`s and is enqueued for consumption by the
 * audio processor.
 */
class AudioInput : public AudioInterface {
public:
  AudioInput(AudioBufferPool *audio_buffer_pool, double desired_sample_rate,
             unsigned long frames_per_buffer = 0);

  virtual ~AudioInput();

private:
  virtual const PaStreamParameters *get_input_parameters();

  /* The instance callback, where we have access to every method/variable in
   * object of class Sine */
  int paCallbackMethod(const void *inputBuffer, void *outputBuffer,
                       unsigned long frameCount,
                       const PaStreamCallbackTimeInfo *timeInfo,
                       PaStreamCallbackFlags statusFlags);

  PaStreamParameters input_parameters_;
};

} // namespace beatled::detector

#endif // BEATDETECTOR_AUDIOINPUT_H