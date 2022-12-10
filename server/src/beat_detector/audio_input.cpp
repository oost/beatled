#include "audio_input.hpp"
#include "../config.hpp"

#include <chrono>

using namespace beat_detector;

AudioInput::AudioInput(AudioBufferPool &audio_buffer_pool, uint32_t sample_rate,
                       uint32_t frames_per_buffer)
    : stream_(0), audio_buffer_pool_{audio_buffer_pool},
      sample_rate_{sample_rate}, frames_per_buffer_{frames_per_buffer} {
  frame_duration_ = (double)frames_per_buffer / sample_rate_;

  // TODO: Add real time scheduling for Linux
}

AudioInput::~AudioInput() {
  SPDLOG_INFO("Destroying audio input");
  stop();
  close();
}

bool AudioInput::open() {
  PaStreamParameters inputParameters;

  int num_apis = Pa_GetHostApiCount();
  SPDLOG_INFO("Host APIs available: {}", num_apis);

  for (int api_idx = 0; api_idx < num_apis; api_idx++) {
    const PaHostApiInfo *api_info = Pa_GetHostApiInfo(api_idx);
    SPDLOG_INFO(" - API {}: type: '{}',  name: '{}', devices: '{}'", api_idx,
                api_info->type, api_info->name, api_info->deviceCount);
  }

  int numDevices = Pa_GetDeviceCount();
  if (numDevices < 0) {
    SPDLOG_ERROR("PortAudio device count: {}", numDevices);
    return false;
  }

  SPDLOG_INFO("PortAudio device count: {}", numDevices);

  inputParameters.device =
      Pa_GetDefaultInputDevice(); /* default input device */
  if (inputParameters.device == paNoDevice) {
    SPDLOG_ERROR("Error: No default input device.");
    return false;
  }

  const PaDeviceInfo *default_input_device_info =
      Pa_GetDeviceInfo(inputParameters.device);

  SPDLOG_INFO("Starting listening on device: {} (API: {}, index: {})",
              default_input_device_info->name,
              default_input_device_info->hostApi, inputParameters.device);

  inputParameters.channelCount = 1;         /* stereo output */
  inputParameters.sampleFormat = paFloat32; /* 32 bit floating point output */
  inputParameters.suggestedLatency =
      default_input_device_info->defaultLowInputLatency;

  inputParameters.hostApiSpecificStreamInfo = NULL;

  int framePerBuffer = paFramesPerBufferUnspecified;
  // int framePerBuffer = constants::audio_buffer_size;

  PaError err = Pa_OpenStream(
      &stream_, &inputParameters, NULL, /* &outputParameters, */
      sample_rate_, framePerBuffer,
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

bool AudioInput::close() {
  if (stream_ == 0)
    return false;
  SPDLOG_INFO("Closing stream");

  PaError err = Pa_CloseStream(stream_);
  stream_ = 0;

  return (err == paNoError);
}

bool AudioInput::start() {
  if (stream_ == 0)
    return false;

  PaError err = Pa_StartStream(stream_);
  SPDLOG_INFO("Starting stream");
  return (err == paNoError);
}

bool AudioInput::is_active() {
  PaError active = Pa_IsStreamActive(stream_);
  if (active == 1) {
    return true;
  }
  if (active < 0) {
    SPDLOG_INFO("Hmm error....");
  }
  return false;
}

bool AudioInput::stop() {
  if (stream_ == 0)
    return false;
  SPDLOG_INFO("Stopping stream\n");

  PaError err = Pa_StopStream(stream_);

  return (err == paNoError);
}

int AudioInput::paCallbackMethod(const void *inputBuffer, void *outputBuffer,
                                 unsigned long framesPerBuffer,
                                 const PaStreamCallbackTimeInfo *timeInfo,
                                 PaStreamCallbackFlags statusFlags) {

  AudioBufferTimespec buffer_start_time = AudioBufferTimespec::now();
  double time_lag = timeInfo->currentTime - timeInfo->inputBufferAdcTime;
  buffer_start_time += time_lag;

  float *input = (float *)inputBuffer;

  if (current_buffer_ == nullptr) {
    current_buffer_ = audio_buffer_pool_.get_new_buffer();

    current_buffer_->set_start_time(buffer_start_time);
  }
  int elements_copied = 0;
  while (elements_copied < framesPerBuffer) {

    elements_copied += current_buffer_->copy_raw_data(
        input + elements_copied, framesPerBuffer - elements_copied);

    if (current_buffer_->is_full()) {
      audio_buffer_pool_.enqueue(std::move(current_buffer_));
      current_buffer_ = audio_buffer_pool_.get_new_buffer();
      buffer_start_time = buffer_start_time + frame_duration_;
      current_buffer_->set_start_time(buffer_start_time);
    }
  }

  (void)outputBuffer;
  (void)timeInfo; /* Prevent unused variable warnings. */
  (void)statusFlags;

  return paContinue;
}

void AudioInput::paStreamFinishedMethod() { SPDLOG_INFO("Stream Completed\n"); }
