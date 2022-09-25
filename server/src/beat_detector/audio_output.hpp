#ifndef BEATDETECTOR_AUDIOOUTPUT_H
#define BEATDETECTOR_AUDIOOUTPUT_H

#include <BTrack.h>
// #include <audiofile.h>
// #include <filesystem>
// #include <iostream>
// #include <math.h>
// #include <portaudio.h>
// #include <stdio.h>
// #include <vector>

#include "../config.hpp"
#include "audio_exception.hpp"

namespace fs = std::filesystem;

namespace beat_detector {

class AudioOutput {
public:
  AudioOutput();
  AudioOutput(std::vector<audio_buffer_t> &&audio_data, double sample_rate,
              int frames_per_buffer);

  ~AudioOutput();

  bool open();
  bool close();
  bool start();
  bool is_active();
  bool stop();

  bool load_from_disk(const std::filesystem::path &file_path);

  const std::vector<audio_buffer_t> &get_audio_data();

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
    return ((AudioOutput *)userData)
        ->paCallbackMethod(inputBuffer, outputBuffer, framesPerBuffer, timeInfo,
                           statusFlags);
  }

  void paStreamFinishedMethod();

  /*
   * This routine is called by portaudio when playback is done.
   */
  static void paStreamFinished(void *userData) {
    return ((AudioOutput *)userData)->paStreamFinishedMethod();
  }

  PaStream *stream_;
  std::vector<audio_buffer_t> audio_data_;
  int read_index_ = 0;
  std::size_t data_length_;
  double sample_rate_;
  int frames_per_buffer_;
  BTrack beat_tracker_;
};

} // namespace beat_detector

#endif // BEATDETECTOR_AUDIOOUTPUT_H