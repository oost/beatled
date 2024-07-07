#include "audio_input.hpp"
#include "beat_detector/audio/config.h"
#include "core/clock.hpp"

#include <chrono>

using namespace beatled::detector;
using beatled::core::Clock;

AudioInput::AudioInput(AudioBufferPool *audio_buffer_pool,
                       double desired_sample_rate,
                       unsigned long frames_per_buffer)
    : AudioInterface(audio_buffer_pool, desired_sample_rate,
                     frames_per_buffer) {}

AudioInput::~AudioInput() {
  SPDLOG_INFO("Destroying audio input");
  stop();
  close();
}

const PaStreamParameters *AudioInput::get_input_parameters() {

  int num_apis = Pa_GetHostApiCount();
  SPDLOG_INFO("Host APIs available: {}", num_apis);

  for (int api_idx = 0; api_idx < num_apis; api_idx++) {
    const PaHostApiInfo *api_info = Pa_GetHostApiInfo(api_idx);
    SPDLOG_INFO(" - API {}: type: '{}',  name: '{}', devices: '{}'", api_idx,
                (int)api_info->type, api_info->name, api_info->deviceCount);
  }

  int numDevices = Pa_GetDeviceCount();
  if (numDevices < 0) {
    SPDLOG_ERROR("PortAudio device count: {}", numDevices);
    throw AudioInputException("No output devices.");
  }

  SPDLOG_INFO("PortAudio device count: {}", numDevices);

  input_parameters_.device =
      Pa_GetDefaultInputDevice(); /* default input device */
  if (input_parameters_.device == paNoDevice) {
    SPDLOG_ERROR("Error: No default input device.");
    throw AudioInputException("No default input device.");
  }

  const PaDeviceInfo *default_input_device_info =
      Pa_GetDeviceInfo(input_parameters_.device);

  SPDLOG_INFO("Starting listening on device: {} (API: {}, index: {})",
              default_input_device_info->name,
              default_input_device_info->hostApi, input_parameters_.device);

  input_parameters_.channelCount = 1;         /* stereo output */
  input_parameters_.sampleFormat = paFloat32; /* 32 bit floating point output */
  input_parameters_.suggestedLatency =
      default_input_device_info->defaultLowInputLatency;

  input_parameters_.hostApiSpecificStreamInfo = NULL;
  return &input_parameters_;
}

int AudioInput::paCallbackMethod(const void *inputBuffer, void *outputBuffer,
                                 unsigned long frameCount,
                                 const PaStreamCallbackTimeInfo *timeInfo,
                                 PaStreamCallbackFlags statusFlags) {

  float *input = (float *)inputBuffer;

  copy_to_buffer(input, frameCount, timeInfo->inputBufferAdcTime);

  (void)outputBuffer;
  (void)statusFlags;

  return paContinue;
}
