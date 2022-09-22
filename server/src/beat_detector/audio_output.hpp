#ifndef BEATDETECTOR_AUDIOOUTPUT_H
#define BEATDETECTOR_AUDIOOUTPUT_H

#include "portaudio.h"
#include <filesystem>
#include <math.h>
#include <stdio.h>
#include <vector>

#include "audiofile.h"
// #define NUM_SECONDS (5)
// #define SAMPLE_RATE (44100)
// #define FRAMES_PER_BUFFER (64)

#ifndef M_PI
#define M_PI (3.14159265)
#endif

#define TABLE_SIZE (200)

namespace beat_detector {

class AudioOutput {
public:
  AudioOutput(std::vector<float> &&audio_data, double sample_rate,
              unsigned long frames_per_buffer)
      : stream_{0}, audio_data_{std::move(audio_data)}, read_index_{0},
        sample_rate_{sample_rate}, frames_per_buffer_{frames_per_buffer} {}

  ~AudioOutput() {
    stop();
    close();
  }

  bool open() {
    PaStreamParameters outputParameters;

    outputParameters.device =
        Pa_GetDefaultOutputDevice(); /* default output device */
    if (outputParameters.device == paNoDevice) {
      fprintf(stderr, "Error: No default output device.\n");
      return false;
    }

    outputParameters.channelCount = 1; /* stereo output */
    outputParameters.sampleFormat =
        paFloat32; /* 32 bit floating point output */
    outputParameters.suggestedLatency =
        Pa_GetDeviceInfo(outputParameters.device)->defaultLowInputLatency;

    outputParameters.hostApiSpecificStreamInfo = NULL;

    PaError err = Pa_OpenStream(
        &stream_, NULL, &outputParameters, /* &outputParameters, */
        sample_rate_, frames_per_buffer_,
        paClipOff, /* we won't output out of range samples so don't bother
                      clipping them */
        &AudioOutput::paCallback, this /* Using 'this' for userData so we can
                                   cast to AudioOutput* in paCallback method */
    );

    if (err != paNoError) {
      /* Failed to open stream to device !!! */
      return false;
    }

    err = Pa_SetStreamFinishedCallback(stream_, &AudioOutput::paStreamFinished);

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

  bool load_from_disk(const std::filesystem::path &file_path) {

    AudioFile<float> audioFile;

    audioFile.load(file_path);

    int sampleRate = audioFile.getSampleRate();
    int bitDepth = audioFile.getBitDepth();

    int numSamples = audioFile.getNumSamplesPerChannel();
    double lengthInSeconds = audioFile.getLengthInSeconds();

    int numChannels = audioFile.getNumChannels();
    bool isMono = audioFile.isMono();
    bool isStereo = audioFile.isStereo();

    // or, just use this quick shortcut to print a summary to the console
    audioFile.printSummary();

    int channel = 0;
    // int numSamples = audioFile.getNumSamplesPerChannel();

    for (int i = 0; i < numSamples; i++) {
      double currentSample = audioFile.samples[channel][i];
    }
    return true;
  }

  // bool save_to_disk(const std::string &file_name) {
  //   // 1. Create an AudioBuffer
  //   // (BTW, AudioBuffer is just a vector of vectors)

  //   AudioFile<float>::AudioBuffer buffer;

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

  //   AudioFile<float> audioFile;
  //   // 5. Put into the AudioFile object
  //   bool ok = audioFile.setAudioBuffer(buffer);

  //   fs::path audio_file_path = fs::current_path() / file_name;
  //   // Wave file (explicit)
  //   return audioFile.save(audio_file_path, AudioFileFormat::Wave);
  // }

  std::vector<float> &&get_audio_data() { return std::move(audio_data_); }

private:
  /* The instance callback, where we have access to every method/variable in
   * object of class Sine */
  int paCallbackMethod(const void *inputBuffer, void *outputBuffer,
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

  void paStreamFinishedMethod() { printf("Stream Completed\n"); }

  /*
   * This routine is called by portaudio when playback is done.
   */
  static void paStreamFinished(void *userData) {
    return ((AudioOutput *)userData)->paStreamFinishedMethod();
  }

  PaStream *stream_;
  std::vector<float> audio_data_;
  int read_index_ = 0;
  std::size_t data_length_;
  double sample_rate_;
  unsigned long frames_per_buffer_;
};

// class Sine {
// public:
//   Sine() : stream(0), left_phase(0), right_phase(0) {
//     /* initialise sinusoidal wavetable */
//     for (int i = 0; i < TABLE_SIZE; i++) {
//       sine[i] = (float)sin(((double)i / (double)TABLE_SIZE) * M_PI * 2.);
//     }

//     sprintf(message, "No Message");
//   }

//   bool open(PaDeviceIndex index) {
//     PaStreamParameters outputParameters;

//     outputParameters.device = index;
//     if (outputParameters.device == paNoDevice) {
//       return false;
//     }

//     const PaDeviceInfo *pInfo = Pa_GetDeviceInfo(index);
//     if (pInfo != 0) {
//       printf("Output device name: '%s'\r", pInfo->name);
//     }

//     outputParameters.channelCount = 2; /* stereo output */
//     outputParameters.sampleFormat =
//         paFloat32; /* 32 bit floating point output */
//     outputParameters.suggestedLatency =
//         Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
//     outputParameters.hostApiSpecificStreamInfo = NULL;

//     PaError err = Pa_OpenStream(
//         &stream, NULL, /* no input */
//         &outputParameters, SAMPLE_RATE, paFramesPerBufferUnspecified,
//         paClipOff, /* we won't output out of range samples so don't bother
//                       clipping them */
//         &Sine::paCallback, this /* Using 'this' for userData so we can cast
//         to
//                                    Sine* in paCallback method */
//     );

//     if (err != paNoError) {
//       /* Failed to open stream to device !!! */
//       return false;
//     }

//     err = Pa_SetStreamFinishedCallback(stream, &Sine::paStreamFinished);

//     if (err != paNoError) {
//       Pa_CloseStream(stream);
//       stream = 0;

//       return false;
//     }

//     return true;
//   }

//   bool close() {
//     if (stream == 0)
//       return false;

//     PaError err = Pa_CloseStream(stream);
//     stream = 0;

//     return (err == paNoError);
//   }

//   bool start() {
//     if (stream == 0)
//       return false;

//     PaError err = Pa_StartStream(stream);

//     return (err == paNoError);
//   }

//   bool stop() {
//     if (stream == 0)
//       return false;

//     PaError err = Pa_StopStream(stream);

//     return (err == paNoError);
//   }

// private:
//   /* The instance callback, where we have access to every method/variable
//   in
//    * object of class Sine */
//   int paCallbackMethod(const void *inputBuffer, void *outputBuffer,
//                        unsigned long framesPerBuffer,
//                        const PaStreamCallbackTimeInfo *timeInfo,
//                        PaStreamCallbackFlags statusFlags) {
//     float *out = (float *)outputBuffer;
//     unsigned long i;

//     (void)timeInfo; /* Prevent unused variable warnings. */
//     (void)statusFlags;
//     (void)inputBuffer;

//     for (i = 0; i < framesPerBuffer; i++) {
//       *out++ = sine[left_phase];  /* left */
//       *out++ = sine[right_phase]; /* right */
//       left_phase += 1;
//       if (left_phase >= TABLE_SIZE)
//         left_phase -= TABLE_SIZE;
//       right_phase += 3; /* higher pitch so we can distinguish left and
//       right.
//       */ if (right_phase >= TABLE_SIZE)
//         right_phase -= TABLE_SIZE;
//     }

//     return paContinue;
//   }

//   /* This routine will be called by the PortAudio engine when audio is
//   needed.
//   ** It may called at interrupt level on some machines so don't do anything
//   ** that could mess up the system like calling malloc() or free().
//   */
//   static int paCallback(const void *inputBuffer, void *outputBuffer,
//                         unsigned long framesPerBuffer,
//                         const PaStreamCallbackTimeInfo *timeInfo,
//                         PaStreamCallbackFlags statusFlags, void *userData)
//                         {
//     /* Here we cast userData to Sine* type so we can call the instance
//     method
//        paCallbackMethod, we can do that since we called Pa_OpenStream with
//        'this' for userData */
//     return ((Sine *)userData)
//         ->paCallbackMethod(inputBuffer, outputBuffer, framesPerBuffer,
//         timeInfo,
//                            statusFlags);
//   }

//   void paStreamFinishedMethod() { printf("Stream Completed: %s\n",
//   message);
//   }

//   /*
//    * This routine is called by portaudio when playback is done.
//    */
//   static void paStreamFinished(void *userData) {
//     return ((Sine *)userData)->paStreamFinishedMethod();
//   }

//   PaStream *stream;
//   float sine[TABLE_SIZE];
//   int left_phase;
//   int right_phase;
//   char message[20];
// };

} // namespace beat_detector

#endif // BEATDETECTOR_AUDIOOUTPUT_H