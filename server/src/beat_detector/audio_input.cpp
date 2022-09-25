#include "audio_input.hpp"
#include "../config.hpp"

using namespace beat_detector;

AudioInput::AudioInput(AudioBufferPool &audio_buffer_pool, double sample_rate,
                       unsigned long frames_per_buffer)
    : stream_(0), audio_buffer_pool_{audio_buffer_pool},
      sample_rate_{sample_rate}, frames_per_buffer_{frames_per_buffer} {}

AudioInput::~AudioInput() {
  stop();
  close();
}

bool AudioInput::open() {
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

bool AudioInput::close() {
  if (stream_ == 0)
    return false;

  PaError err = Pa_CloseStream(stream_);
  stream_ = 0;

  return (err == paNoError);
}

bool AudioInput::start() {
  if (stream_ == 0)
    return false;

  PaError err = Pa_StartStream(stream_);
  printf("Starting stream\n");
  return (err == paNoError);
}

bool AudioInput::is_active() {
  PaError active = Pa_IsStreamActive(stream_);
  if (active == 1) {
    return true;
  }
  if (active < 0) {
    printf("Hmm error....");
  }
  return false;
}

bool AudioInput::stop() {
  if (stream_ == 0)
    return false;

  PaError err = Pa_StopStream(stream_);

  return (err == paNoError);
}

// std::string AudioInput::save_to_disk(const std::string &file_name) {
//   // 1. Create an AudioBuffer
//   // (BTW, AudioBuffer is just a vector of vectors)

//   AudioFile<audio_buffer_t>::AudioBuffer buffer;

//   // 2. Set to (e.g.) two channels
//   buffer.resize(1);

//   // 3. Set number of samples per channel
//   buffer[0].resize(audio_data_.size());

//   // 4. do something here to fill the buffer with samples, e.g.

//   // then...

//   int numChannels = 1;
//   int numSamplesPerChannel = audio_data_.size();
//   float sampleRate = 44100.f;
//   float frequency = 440.f;

//   for (int i = 0; i < numSamplesPerChannel; i++) {
//     for (int channel = 0; channel < numChannels; channel++)
//       buffer[0][i] = audio_data_[i];
//   }

//   AudioFile<audio_buffer_t> audioFile;
//   // 5. Put into the AudioFile object
//   if (!audioFile.setAudioBuffer(buffer)) {
//     throw AudioInputException("Error handling audio buffer.");
//   }

//   fs::path audio_file_path = file_name;

//   if (audio_file_path.is_relative()) {
//     audio_file_path = fs::current_path() / file_name;
//   }

//   std::cout << "Saving audio file to: " << audio_file_path << std::endl;
//   // Wave file (explicit)
//   if (!audioFile.save(audio_file_path, AudioFileFormat::Wave)) {
//     throw AudioFileException("Error saving file.");
//   }
//   return audio_file_path;
// }

// const std::vector<audio_buffer_t> &AudioInput::get_audio_data() {
//   std::cout << "audio_data.length: " << audio_data_.size() << std::endl;
//   return audio_data_;
// }

int AudioInput::paCallbackMethod(const void *inputBuffer, void *outputBuffer,
                                 unsigned long framesPerBuffer,
                                 const PaStreamCallbackTimeInfo *timeInfo,
                                 PaStreamCallbackFlags statusFlags) {

  float *input = (float *)inputBuffer;
  if (current_buffer_ == nullptr) {
    current_buffer_ = audio_buffer_pool_.get_new_buffer();
  }
  int elements_copied = 0;
  while (elements_copied < framesPerBuffer) {

    elements_copied += current_buffer_->copy_raw_data(
        input + elements_copied, framesPerBuffer - elements_copied);
    if (current_buffer_->is_full()) {
      audio_buffer_pool_.enqueue(std::move(current_buffer_));
      current_buffer_ = audio_buffer_pool_.get_new_buffer();
    }
  }

  (void)outputBuffer;
  (void)timeInfo; /* Prevent unused variable warnings. */
  (void)statusFlags;

  return paContinue;
}

void AudioInput::paStreamFinishedMethod() { printf("Stream Completed\n"); }
