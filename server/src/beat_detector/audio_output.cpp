

#include <BTrack.h>
#include <audiofile.h>
#include <filesystem>
#include <iostream>
#include <math.h>
#include <portaudio.h>
#include <spdlog/spdlog.h>
#include <stdio.h>
#include <vector>

#include "../config.hpp"
#include "audio_exception.hpp"
#include "audio_output.hpp"

namespace fs = std::filesystem;

using namespace beat_detector;

AudioOutput::AudioOutput(std::vector<audio_buffer_t> &audio_data,
                         uint32_t sample_rate)
    : stream_{0}, audio_data_{std::move(audio_data)}, read_index_{0},
      sample_rate_{sample_rate}, beat_tracker_{constants::audio_buffer_size} {}

AudioOutput::~AudioOutput() {
  stop();
  close();
}

bool AudioOutput::open() {
  PaStreamParameters outputParameters;

  outputParameters.device =
      Pa_GetDefaultOutputDevice(); /* default output device */
  if (outputParameters.device == paNoDevice) {
    fprintf(stderr, "Error: No default output device.\n");
    throw AudioException("No default output device.\n");
  }

  outputParameters.channelCount = 1;         /* stereo output */
  outputParameters.sampleFormat = paFloat32; /* 32 bit floating point output */
  outputParameters.suggestedLatency =
      Pa_GetDeviceInfo(outputParameters.device)->defaultLowInputLatency;

  outputParameters.hostApiSpecificStreamInfo = NULL;

  PaError err = Pa_OpenStream(
      &stream_, NULL, &outputParameters, /* &outputParameters, */
      sample_rate_, 512, paClipOff,  /* we won't output out of range samples so
                                      don't bother    clipping them */
      &AudioOutput::paCallback, this /* Using 'this' for userData so we can
                                 cast to AudioOutput* in paCallback method */
  );

  if (err != paNoError) {
    /* Failed to open stream to device !!! */
    throw AudioException("Failed to open stream to device !!!");
  }

  err = Pa_SetStreamFinishedCallback(stream_, &AudioOutput::paStreamFinished);

  if (err != paNoError) {
    Pa_CloseStream(stream_);
    stream_ = 0;
    throw AudioException("Failed to set finished callback");
  }

  return true;
}

bool AudioOutput::close() {
  if (stream_ == 0)
    return false;

  PaError err = Pa_CloseStream(stream_);
  stream_ = 0;

  return (err == paNoError);
}

bool AudioOutput::start() {
  if (stream_ == 0)
    return false;

  PaError err = Pa_StartStream(stream_);
  printf("Starting stream\n");
  return (err == paNoError);
}

bool AudioOutput::is_active() {
  PaError active = Pa_IsStreamActive(stream_);
  if (active == 1) {
    return true;
  }
  if (active < 0) {
    printf("Hmm error....");
  }
  return false;
}

bool AudioOutput::stop() {
  if (stream_ == 0)
    return false;

  PaError err = Pa_StopStream(stream_);

  return (err == paNoError);
}

// bool AudioOutput::load_from_disk(const std::filesystem::path &file_path) {
//   //---------------------------------------------------------------
//   // 1. Set a file path to an audio file on your machine
//   fs::path filePath = file_path;

//   if (filePath.is_relative()) {
//     filePath = fs::current_path() / filePath;
//   }

//   //---------------------------------------------------------------
//   // 2. Create an AudioFile object and load the audio file

//   AudioFile<audio_buffer_t> audio_file;
//   bool loadedOK = audio_file.load(filePath);

//   /** If you hit this assert then the file path above
//    probably doesn't refer to a valid audio file */
//   assert(loadedOK);

//   sample_rate_ = audio_file.getSampleRate();

//   // or, just use this quick shortcut to print a summary to the console
//   audio_file.printSummary();

//   if (audio_file.getNumChannels() > 1) {
//     throw AudioException("Multiple channels not supported (yet)...");
//   }

//   int channel = 0;

//   audio_data_.resize(audio_file.getNumSamplesPerChannel());
//   for (int i = 0; i < audio_file.getNumSamplesPerChannel(); i++) {
//     audio_data_[i] = audio_file.samples[channel][i];
//   }
//   return true;
// }

// const std::vector<audio_buffer_t> &AudioOutput::get_audio_data() {
//   return std::move(audio_data_);
// }

/* The instance callback, where we have access to every method/variable in
 * object of class Sine */
int AudioOutput::paCallbackMethod(const void *inputBuffer, void *outputBuffer,
                                  unsigned long framesPerBuffer,
                                  const PaStreamCallbackTimeInfo *timeInfo,
                                  PaStreamCallbackFlags statusFlags) {

  float *out = (float *)outputBuffer;

  std::size_t elements_readable = audio_data_.size() - read_index_;
  bool last_segment = (elements_readable < framesPerBuffer);
  int elements_to_read = last_segment ? elements_readable : framesPerBuffer;

  for (int i = 0; i < elements_to_read; i++) {
    out[i] = audio_data_[read_index_ + i];
  }

  double *hop_data = audio_data_.data();
  beat_tracker_.processAudioFrame(hop_data + read_index_);

  if (beat_tracker_.beatDueInCurrentFrame()) {
    // do something on the beat
    SPDLOG_INFO("Beat --- ");
  }
  // printf("Writing at %d, %d (frames %lu)", read_index_, elements_to_read,
  //        framesPerBuffer);

  read_index_ = read_index_ + elements_to_read;

  (void)timeInfo; /* Prevent unused variable warnings. */
  (void)statusFlags;
  (void)inputBuffer;

  if (last_segment) {
    return paComplete;
  }

  return paContinue;
}

void AudioOutput::paStreamFinishedMethod() { printf("Stream Completed\n"); }
