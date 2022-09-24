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

#include "../config.h"
#include "audio_buffer_pool.hpp"
#include "audio_exception.hpp"

#define TABLE_SIZE (200)

namespace fs = std::filesystem;

namespace beat_detector {

class AudioInput {
public:
  AudioInput(std::size_t data_length, double sample_rate,
             unsigned long frames_per_buffer)
      : stream_(0), data_length_{data_length},
        audio_data_(data_length, 0.0), write_idx_{0}, sample_rate_{sample_rate},
        frames_per_buffer_{frames_per_buffer} {

    std::cout << "audio_data.length: " << audio_data_.size() << std::endl;
  }

  ~AudioInput() {
    stop();
    close();
  }

  bool open() {
    PaStreamParameters inputParameters;

    inputParameters.device =
        Pa_GetDefaultInputDevice(); /* default input device */
    if (inputParameters.device == paNoDevice) {
      fprintf(stderr, "Error: No default input device.\n");
      return false;
    }

    inputParameters.channelCount = 1;         /* stereo output */
    inputParameters.sampleFormat = paFloat32; /* 32 bit floating point output */
    inputParameters.suggestedLatency =
        Pa_GetDeviceInfo(inputParameters.device)->defaultLowInputLatency;

    inputParameters.hostApiSpecificStreamInfo = NULL;

    PaError err = Pa_OpenStream(
        &stream_, &inputParameters, NULL, /* &outputParameters, */
        sample_rate_, frames_per_buffer_,
        paClipOff, /* we won't output out of range samples so don't bother
                      clipping them */
        &AudioInput::paCallback, this /* Using 'this' for userData so we can
                                   cast to AudioInput* in paCallback method */
    );

    if (err != paNoError) {
      /* Failed to open stream to device !!! */
      return false;
    }

    err = Pa_SetStreamFinishedCallback(stream_, &AudioInput::paStreamFinished);

    if (err != paNoError) {
      Pa_CloseStream(stream_);
      stream_ = 0;

      return false;
    }

    return true;
  }

  bool close() {
    if (stream_ == 0)
      return false;

    PaError err = Pa_CloseStream(stream_);
    stream_ = 0;

    return (err == paNoError);
  }

  bool start() {
    if (stream_ == 0)
      return false;

    PaError err = Pa_StartStream(stream_);
    printf("Starting stream\n");
    return (err == paNoError);
  }

  bool is_active() {
    PaError active = Pa_IsStreamActive(stream_);
    if (active == 1) {
      return true;
    }
    if (active < 0) {
      printf("Hmm error....");
    }
    return false;
  }

  bool stop() {
    if (stream_ == 0)
      return false;

    PaError err = Pa_StopStream(stream_);

    return (err == paNoError);
  }

  std::string save_to_disk(const std::string &file_name) {
    // 1. Create an AudioBuffer
    // (BTW, AudioBuffer is just a vector of vectors)

    AudioFile<audio_buffer_t>::AudioBuffer buffer;

    // 2. Set to (e.g.) two channels
    buffer.resize(1);

    // 3. Set number of samples per channel
    buffer[0].resize(audio_data_.size());

    // 4. do something here to fill the buffer with samples, e.g.

    // then...

    int numChannels = 1;
    int numSamplesPerChannel = audio_data_.size();
    float sampleRate = 44100.f;
    float frequency = 440.f;

    for (int i = 0; i < numSamplesPerChannel; i++) {
      for (int channel = 0; channel < numChannels; channel++)
        buffer[0][i] = audio_data_[i];
    }

    AudioFile<audio_buffer_t> audioFile;
    // 5. Put into the AudioFile object
    if (!audioFile.setAudioBuffer(buffer)) {
      throw AudioInputException("Error handling audio buffer.");
    }

    fs::path audio_file_path = file_name;

    if (audio_file_path.is_relative()) {
      audio_file_path = fs::current_path() / file_name;
    }

    std::cout << "Saving audio file to: " << audio_file_path << std::endl;
    // Wave file (explicit)
    if (!audioFile.save(audio_file_path, AudioFileFormat::Wave)) {
      throw AudioFileException("Error saving file.");
    }
    return audio_file_path;
  }

  std::vector<audio_buffer_t> &&get_audio_data() {
    std::cout << "audio_data.length: " << audio_data_.size() << std::endl;
    return std::move(audio_data_);
  }

private:
  /* The instance callback, where we have access to every method/variable in
   * object of class Sine */
  int paCallbackMethod(const void *inputBuffer, void *outputBuffer,
                       unsigned long framesPerBuffer,
                       const PaStreamCallbackTimeInfo *timeInfo,
                       PaStreamCallbackFlags statusFlags) {

    float *input = (float *)inputBuffer;
    std::size_t elements_writeable = data_length_ - write_idx_;

    bool last_segment = (elements_writeable < framesPerBuffer);
    int elements_to_write = last_segment ? elements_writeable : framesPerBuffer;

    for (int i = 0; i < elements_to_write; i++) {
      audio_data_[write_idx_ + i] = input[i];
    }

    write_idx_ += elements_to_write;

    (void)outputBuffer;
    (void)timeInfo; /* Prevent unused variable warnings. */
    (void)statusFlags;

    if (last_segment) {
      return paComplete;
    }

    return paContinue;
  }

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

  void paStreamFinishedMethod() { printf("Stream Completed\n"); }

  /*
   * This routine is called by portaudio when playback is done.
   */
  static void paStreamFinished(void *userData) {
    return ((AudioInput *)userData)->paStreamFinishedMethod();
  }

  PaStream *stream_;
  std::vector<audio_buffer_t> audio_data_;
  int write_idx_ = 0;
  std::size_t data_length_;
  double sample_rate_;
  unsigned long frames_per_buffer_;
};

} // namespace beat_detector

#endif // BEATDETECTOR_AUDIOINPUT_H