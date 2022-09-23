#include "beat_detector/audio_input.hpp"
#include "beat_detector/audio_output.hpp"
#include "beat_detector/portaudio_handler.hpp"
#include <portaudio.h>

#include <iostream>

using namespace beat_detector;

/*******************************************************************/
int main(void) {

  ScopedPaHandler paInit;
  if (paInit.result() != paNoError)
    return 1;

  constexpr double SAMPLE_RATE = 44100;
  constexpr unsigned long FRAMES_PER_BUFFER = 1024;
  constexpr double NUM_SECONDS = 2;
  constexpr unsigned long BUFFER_SIZE = SAMPLE_RATE * NUM_SECONDS;
  std::cout << "Buffer size: " << BUFFER_SIZE << std::endl;

  AudioInput audio_input(BUFFER_SIZE, SAMPLE_RATE, FRAMES_PER_BUFFER);

  if (!audio_input.open()) {
    std::cout << "Couldn't open device." << std::endl;
    return 1;
  }

  if (!audio_input.start()) {
    std::cout << "Couldn't start stream." << std::endl;
    return 1;
  }

  std::cout << "Audio input active: " << audio_input.is_active() << std::endl;

  Pa_Sleep(NUM_SECONDS * 1000 + 100);

  // Add thread syncronization

  std::cout << "Audio input active: " << audio_input.is_active() << std::endl;

  audio_input.save_to_disk("audioFile.wav");

  std::vector<audio_buffer_t> ad = audio_input.get_audio_data();

  if (!audio_input.stop()) {
    std::cout << "Couldn't stop stream." << std::endl;
    return 1;
  }

  if (!audio_input.close()) {
    std::cout << "Couldn't close stream." << std::endl;
    return 1;
  }

  std::cout << "Input stream closed ! Got a vector with " << ad.size()
            << " values" << std::endl;

  AudioOutput audio_output(std::move(ad), SAMPLE_RATE, FRAMES_PER_BUFFER);

  if (!audio_output.open()) {
    std::cout << "Couldn't open device." << std::endl;
    return 1;
  }

  std::cout << "Output stream opened ! " << std::endl;

  if (!audio_output.start()) {
    std::cout << "Couldn't start stream." << std::endl;
    return 1;
  }

  std::cout << "Audio output active: " << audio_output.is_active() << std::endl;

  Pa_Sleep(NUM_SECONDS * 1000 + 100);

  // Add thread syncronization

  std::cout << "Audio output active: " << audio_output.is_active() << std::endl;

  audio_output.close();

  // for (auto d : ad) {
  //   std::cout << d << std::endl;
  // }
  // printf("PortAudio Test: output sine wave. SR = %d, BufSize = %d\n",
  //      SAMPLE_RATE, FRAMES_PER_BUFFER);
  //   Sine sine;
  //   if (sine.open(Pa_GetDefaultOutputDevice())) {
  //     if (sine.start()) {
  //       printf("Play for %d seconds.\n", NUM_SECONDS);
  //       Pa_Sleep(NUM_SECONDS * 1000);

  //       sine.stop();
  //     }

  //     sine.close();
  //   }

  //   printf("Test finished.\n");
  //   return paNoError;

  // error:
  //   fprintf(stderr, "An error occurred while using the portaudio stream\n");
  //   fprintf(stderr, "Error number: %d\n", paInit.result());
  //   fprintf(stderr, "Error message: %s\n", Pa_GetErrorText(paInit.result()));
  //   return 1;
}