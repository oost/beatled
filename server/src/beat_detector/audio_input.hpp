#ifndef BEATDETECTOR_AUDIOINPUT_H
#define BEATDETECTOR_AUDIOINPUT_H

#include "portaudio.h"
#include <math.h>
#include <stdio.h>

#define NUM_SECONDS (5)
#define SAMPLE_RATE (44100)
#define FRAMES_PER_BUFFER (64)

#ifndef M_PI
#define M_PI (3.14159265)
#endif

#define TABLE_SIZE (200)

namespace beat_detector {
class Sine {
public:
  Sine() : stream(0), left_phase(0), right_phase(0) {
    /* initialise sinusoidal wavetable */
    for (int i = 0; i < TABLE_SIZE; i++) {
      sine[i] = (float)sin(((double)i / (double)TABLE_SIZE) * M_PI * 2.);
    }

    sprintf(message, "No Message");
  }

  bool open(PaDeviceIndex index) {
    PaStreamParameters outputParameters;

    outputParameters.device = index;
    if (outputParameters.device == paNoDevice) {
      return false;
    }

    const PaDeviceInfo *pInfo = Pa_GetDeviceInfo(index);
    if (pInfo != 0) {
      printf("Output device name: '%s'\r", pInfo->name);
    }

    outputParameters.channelCount = 2; /* stereo output */
    outputParameters.sampleFormat =
        paFloat32; /* 32 bit floating point output */
    outputParameters.suggestedLatency =
        Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;

    PaError err = Pa_OpenStream(
        &stream, NULL, /* no input */
        &outputParameters, SAMPLE_RATE, paFramesPerBufferUnspecified,
        paClipOff, /* we won't output out of range samples so don't bother
                      clipping them */
        &Sine::paCallback, this /* Using 'this' for userData so we can cast to
                                   Sine* in paCallback method */
    );

    if (err != paNoError) {
      /* Failed to open stream to device !!! */
      return false;
    }

    err = Pa_SetStreamFinishedCallback(stream, &Sine::paStreamFinished);

    if (err != paNoError) {
      Pa_CloseStream(stream);
      stream = 0;

      return false;
    }

    return true;
  }

  bool close() {
    if (stream == 0)
      return false;

    PaError err = Pa_CloseStream(stream);
    stream = 0;

    return (err == paNoError);
  }

  bool start() {
    if (stream == 0)
      return false;

    PaError err = Pa_StartStream(stream);

    return (err == paNoError);
  }

  bool stop() {
    if (stream == 0)
      return false;

    PaError err = Pa_StopStream(stream);

    return (err == paNoError);
  }

private:
  /* The instance callback, where we have access to every method/variable in
   * object of class Sine */
  int paCallbackMethod(const void *inputBuffer, void *outputBuffer,
                       unsigned long framesPerBuffer,
                       const PaStreamCallbackTimeInfo *timeInfo,
                       PaStreamCallbackFlags statusFlags) {
    float *out = (float *)outputBuffer;
    unsigned long i;

    (void)timeInfo; /* Prevent unused variable warnings. */
    (void)statusFlags;
    (void)inputBuffer;

    for (i = 0; i < framesPerBuffer; i++) {
      *out++ = sine[left_phase];  /* left */
      *out++ = sine[right_phase]; /* right */
      left_phase += 1;
      if (left_phase >= TABLE_SIZE)
        left_phase -= TABLE_SIZE;
      right_phase += 3; /* higher pitch so we can distinguish left and right. */
      if (right_phase >= TABLE_SIZE)
        right_phase -= TABLE_SIZE;
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
    return ((Sine *)userData)
        ->paCallbackMethod(inputBuffer, outputBuffer, framesPerBuffer, timeInfo,
                           statusFlags);
  }

  void paStreamFinishedMethod() { printf("Stream Completed: %s\n", message); }

  /*
   * This routine is called by portaudio when playback is done.
   */
  static void paStreamFinished(void *userData) {
    return ((Sine *)userData)->paStreamFinishedMethod();
  }

  PaStream *stream;
  float sine[TABLE_SIZE];
  int left_phase;
  int right_phase;
  char message[20];
};

} // namespace beat_detector

#endif // BEATDETECTOR_AUDIOINPUT_H