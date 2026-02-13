#include "audio_interface.hpp"
#include "beat_detector/audio/config.h"
#include "core/clock.hpp"

#include <chrono>

using namespace beatled::detector;
using beatled::core::Clock;

AudioInterface::AudioInterface(AudioBufferPool *audio_buffer_pool,
                               double desired_sample_rate,
                               unsigned long frames_per_buffer)
    : stream_(nullptr), audio_buffer_pool_{audio_buffer_pool},
      sample_rate_{desired_sample_rate}, frames_per_buffer_{frames_per_buffer} {
  if (frames_per_buffer_ == 0) {
    frames_per_buffer_ = paFramesPerBufferUnspecified;
  }
  // TODO: Add real time scheduling for Linux
}

AudioInterface::~AudioInterface() {
  SPDLOG_INFO("Destroying audio interface");
  stop();
  close();
}

void AudioInterface::log_parameters(const PaStreamParameters *parameters) {
  if (!parameters) {
    SPDLOG_INFO("   - No device connected");
    return;
  }
  SPDLOG_INFO("   - device: {}", parameters->device);
  SPDLOG_INFO("   - channelCount: {}", parameters->channelCount);
  SPDLOG_INFO("   - sampleFormat: {}", parameters->sampleFormat);
  SPDLOG_INFO("   - suggestedLatency: {}", parameters->suggestedLatency);
}

bool AudioInterface::open() {
  const PaStreamParameters *input_parameters = get_input_parameters();
  const PaStreamParameters *output_parameters = get_output_parameters();
  SPDLOG_INFO("Starting PortAudio stream with");
  SPDLOG_INFO(" - sampleRate: {}", sample_rate_);
  SPDLOG_INFO(" - framesPerBuffer: {}", frames_per_buffer_);
  SPDLOG_INFO(" - Input Device:");
  log_parameters(input_parameters);
  SPDLOG_INFO(" - Output Device:");
  log_parameters(output_parameters);

  PaError err = Pa_OpenStream(
      &stream_, input_parameters, output_parameters, /* &outputParameters, */
      sample_rate_, frames_per_buffer_,
      paClipOff,                        /* we won't output out of range samples
                                           so don't bother clipping them */
      &AudioInterface::paCallback, this /* Using 'this' for userData so we can
                                 cast to AudioInterface* in paCallback method */
  );

  const PaStreamInfo *stream_info = Pa_GetStreamInfo(stream_);

  SPDLOG_INFO("Stream info: input latency {}, sample rate {}",
              stream_info->inputLatency, stream_info->sampleRate);

  sample_rate_ = stream_info->sampleRate;
  frame_duration_ =
      static_cast<double>(audio_buffer_pool_->buffer_size()) / sample_rate_;

  audio_buffer_pool_->set_sample_rate(sample_rate_);

  if (err != paNoError) {
    /* Failed to open stream to device !!! */
    return false;
  }

  err =
      Pa_SetStreamFinishedCallback(stream_, &AudioInterface::paStreamFinished);

  if (err != paNoError) {
    Pa_CloseStream(stream_);
    stream_ = nullptr;

    return false;
  }

  return true;
}

bool AudioInterface::close() {
  if (stream_ == nullptr)
    return false;
  SPDLOG_INFO("Closing stream");

  PaError err = Pa_CloseStream(stream_);
  stream_ = nullptr;

  return (err == paNoError);
}

bool AudioInterface::start() {
  if (stream_ == nullptr)
    return false;

  current_buffer_ = audio_buffer_pool_->get_new_buffer();
  stream_start_time_ = Clock::time_us_64();
  stream_start_timeInfo_ = Pa_GetStreamTime(stream_);
  SPDLOG_INFO("stream_start_time_ {},  stream_start_timeInfo_ {}",
              stream_start_time_, stream_start_timeInfo_);

  PaError err = Pa_StartStream(stream_);
  SPDLOG_INFO("Starting stream");

  return (err == paNoError);
}

bool AudioInterface::is_active() {
  PaError active = Pa_IsStreamActive(stream_);
  if (active == 1) {
    return true;
  }
  if (active < 0) {
    SPDLOG_INFO("Hmm error....");
  }
  return false;
}

bool AudioInterface::wait() {
  PaError err;
  SPDLOG_INFO("Waiting for stream to finish");
  while ((err = Pa_IsStreamActive(stream_)) == 1)
    Pa_Sleep(100);
  SPDLOG_INFO("Stream is finished!");
  return (err == paNoError);
}

bool AudioInterface::stop() {
  if (stream_ == nullptr)
    return false;
  SPDLOG_INFO("Stopping stream");

  PaError err = Pa_StopStream(stream_);

  return (err == paNoError);
}

void AudioInterface::copy_to_buffer(float *input_buffer,
                                    unsigned long frame_count,
                                    double input_output_time,
                                    double current_time) {

  PaTime stream_time = Pa_GetStreamTime(stream_);
  SPDLOG_INFO("Copy to Buffer {:.4f} {:.4f} {:4f}", input_output_time,
              current_time, stream_time);

  int elements_copied = 0;
  while (elements_copied < frame_count) {
    // If we have a new buffer, let's set the start time
    if (current_buffer_->start_time() == 0) {
      // Set time here...
      uint64_t now = Clock::time_us_64();
      uint64_t buffer_time =
          now - static_cast<uint64_t>(1e6 * (stream_time - input_output_time));
      current_buffer_->set_start_time(buffer_time);
      // SPDLOG_INFO("Setting start time. Elements copied {}, frame count {}",
      //             elements_copied, frame_count);
    }

    elements_copied += current_buffer_->copy_raw_data(
        input_buffer + elements_copied, frame_count - elements_copied);

    // If buffer is full, let's get a new one
    if (current_buffer_->is_full()) {
      audio_buffer_pool_->enqueue(std::move(current_buffer_));
      current_buffer_ = audio_buffer_pool_->get_new_buffer();
    }
  }
}

void AudioInterface::paStreamFinishedMethod() {
  SPDLOG_INFO("Stream Completed");
  audio_buffer_pool_->release_buffer(std::move(current_buffer_));
}
