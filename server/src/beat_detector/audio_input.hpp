#ifndef BEATDETECTOR_AUDIOINPUT_H
#define BEATDETECTOR_AUDIOINPUT_H

#include <AudioFile.h>
#include <exception>
#include <filesystem>
#include <iostream>
#include <math.h>
#include <portaudio.h>
#include <stdio.h>
#include <vector>

#include "../config.hpp"
#include "audio_buffer_pool.hpp"
#include "audio_exception.hpp"
#include "portaudio_handler.hpp"

namespace fs = std::filesystem;

namespace beat_detector {

class AudioInput {
public:
  AudioInput(AudioBufferPool &audio_buffer_pool, uint32_t sample_rate,
             uint32_t frames_per_buffer);

  ~AudioInput();

  bool open();
  bool close();
  bool start();
  bool is_active();
  bool stop();

private:
  /* The instance callback, where we have access to every method/variable in
   * object of class Sine */
  int paCallbackMethod(const void *inputBuffer, void *outputBuffer,
                       unsigned long framesPerBuffer,
                       const PaStreamCallbackTimeInfo *timeInfo,
                       PaStreamCallbackFlags statusFlags);

  /* This routine will be called by the PortAudio engine when audio is needed.
  ** It may called at interrupt level on some machines so don't do anything
  ** that could mess up the system like calling malloc() or free().
  */
  static int paCallback(const void *inputBuffer, void *outputBuffer,
                        unsigned long framesPerBuffer,
                        const PaStreamCallbackTimeInfo *timeInfo,
                        PaStreamCallbackFlags statusFlags, void *userData) {
    /* Here we cast userData to Sine* type so we can call the instance method
       paCallbackMethod, we can do that since we called Pa_OpenStream with
       'this' for userData */
    return ((AudioInput *)userData)
        ->paCallbackMethod(inputBuffer, outputBuffer, framesPerBuffer, timeInfo,
                           statusFlags);
  }

  void paStreamFinishedMethod();

  /*
   * This routine is called by portaudio when playback is done.
   */
  static void paStreamFinished(void *userData) {
    return ((AudioInput *)userData)->paStreamFinishedMethod();
  }

  ScopedPaHandler port_audio_handler_;
  PaStream *stream_;
  AudioBufferPool &audio_buffer_pool_;
  uint32_t sample_rate_;
  uint32_t frames_per_buffer_;
  AudioBuffer::Ptr current_buffer_;
  double frame_duration_;
};

} // namespace beat_detector

#endif // BEATDETECTOR_AUDIOINPUT_H