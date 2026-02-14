

#include <AudioFile/AudioFile.h>
#include <beat_tracker.hpp>
#include <filesystem>
#include <iostream>
#include <math.h>
#include <portaudio.h>
#include <spdlog/spdlog.h>
#include <stdio.h>
#include <vector>

#include "audio_exception.hpp"
#include "audio_output.hpp"
#include "beat_detector/audio/config.h"

namespace fs = std::filesystem;

using namespace beatled::detector;

AudioOutput::AudioOutput(std::vector<audio_buffer_t> &audio_data,
                         AudioBufferPool *audio_buffer_pool,
                         uint32_t sample_rate, std::size_t audio_buffer_size,
                         unsigned long frames_per_buffer)
    : AudioInterface(audio_buffer_pool, sample_rate, frames_per_buffer),
      audio_data_{std::move(audio_data)}, read_index_{0} {}

const PaStreamParameters *AudioOutput::get_output_parameters() {

  output_parameters_.device =
      Pa_GetDefaultOutputDevice(); /* default output device */
  if (output_parameters_.device == paNoDevice) {
    SPDLOG_ERROR("Error: No default output device.");
    throw AudioException("No default output device.");
  }

  output_parameters_.channelCount = 1; /* stereo output */
  output_parameters_.sampleFormat =
      paFloat32; /* 32 bit floating point output */
  output_parameters_.suggestedLatency =
      Pa_GetDeviceInfo(output_parameters_.device)->defaultLowInputLatency;

  output_parameters_.hostApiSpecificStreamInfo = NULL;
  return &output_parameters_;
}
int AudioOutput::paCallbackMethod(const void *inputBuffer, void *outputBuffer,
                                  unsigned long frameCount,
                                  const PaStreamCallbackTimeInfo *timeInfo,
                                  PaStreamCallbackFlags statusFlags) {

  float *out = (float *)outputBuffer;

  std::size_t elements_readable = audio_data_.size() - read_index_;
  bool last_segment = (elements_readable < frameCount);
  int elements_to_read = last_segment ? elements_readable : frameCount;

  for (int i = 0; i < elements_to_read; i++) {
    out[i] = audio_data_[read_index_ + i];
  }

  double *hop_data = audio_data_.data();
  auto audio_frame = std::span<double>(
      hop_data + read_index_, hop_data + read_index_ + elements_to_read);

  read_index_ = read_index_ + elements_to_read;

  copy_to_buffer(out, frameCount, timeInfo->outputBufferDacTime,
                 timeInfo->currentTime);

  (void)statusFlags; /* Prevent unused variable warnings. */
  (void)inputBuffer;

  if (last_segment) {
    return paComplete;
  }

  return paContinue;
}