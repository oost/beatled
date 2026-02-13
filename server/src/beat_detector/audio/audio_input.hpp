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

  // /**
  //  * @brief Opens the audio stream
  //  * @return true on successful opening of the stream
  //  */
  // bool open();

  // /**
  //  * @brief Closes the audio stream
  //  * @return true on successful closing of the stream
  //  */
  // bool close();

  // /**
  //  * @brief Starts the audio stream
  //  * @return true on successful streaming of the stream
  //  */
  // bool start();

  // /**
  //  * @brief Indicates whether the audio stream is currently active
  //  * @return true on active
  //  */
  // bool is_active();

  // bool wait();

  // /**
  //  * @brief Stops the audio stream
  //  * @return true on successful stropping of the stream
  //  */
  // bool stop();

  // /**
  //  * @brief Returns the effective sample rate
  //  * @return Value of the sample rate that PortAudio is sampling at
  //  */
  // double effective_sample_rate() const { return sample_rate_; }

private:
  virtual const PaStreamParameters *get_input_parameters();

  /* The instance callback, where we have access to every method/variable in
   * object of class Sine */
  int paCallbackMethod(const void *inputBuffer, void *outputBuffer,
                       unsigned long frameCount,
                       const PaStreamCallbackTimeInfo *timeInfo,
                       PaStreamCallbackFlags statusFlags);

  // /* This routine will be called by the PortAudio engine when audio is
  // needed.
  // ** It may called at interrupt level on some machines so don't do anything
  // ** that could mess up the system like calling malloc() or free().
  // */
  // static int paCallback(const void *inputBuffer, void *outputBuffer,
  //                       unsigned long framesPerBuffer,
  //                       const PaStreamCallbackTimeInfo *timeInfo,
  //                       PaStreamCallbackFlags statusFlags, void *userData) {
  //   /* Here we cast userData to Sine* type so we can call the instance method
  //      paCallbackMethod, we can do that since we called Pa_OpenStream with
  //      'this' for userData */
  //   return ((AudioInput *)userData)
  //       ->paCallbackMethod(inputBuffer, outputBuffer, framesPerBuffer,
  //       timeInfo,
  //                          statusFlags);
  // }

  // void paStreamFinishedMethod();

  // /*
  //  * This routine is called by portaudio when playback is done.
  //  */
  // static void paStreamFinished(void *userData) {
  //   return ((AudioInput *)userData)->paStreamFinishedMethod();
  // }

  // PortaudioHandle port_audio_handler_;

  // /**
  //  * @brief A PortAudio stream object
  //  */
  // PaStream *stream_;

  // /**
  //  * @brief The AudioBufferPool to get and enqueue `AudioBuffer`s
  //  */
  // AudioBufferPool *audio_buffer_pool_;

  // /**
  //  * @brief The sample rate of the input
  //  */
  // double sample_rate_;

  // unsigned long frames_per_buffer_;

  // AudioBuffer::Ptr current_buffer_;
  // double frame_duration_;

  // uint64_t stream_start_time_;
  // PaTime stream_start_timeInfo_;

  PaStreamParameters input_parameters_;
};

} // namespace beatled::detector

#endif // BEATDETECTOR_AUDIOINPUT_H