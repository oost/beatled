#include "beat_detector/audio_input.hpp"
#include "beat_detector/portaudio_handler.hpp"
#include "portaudio.h"

using namespace beat_detector;

/*******************************************************************/
int main(void);
int main(void) {
  Sine sine;

  printf("PortAudio Test: output sine wave. SR = %d, BufSize = %d\n",
         SAMPLE_RATE, FRAMES_PER_BUFFER);

  ScopedPaHandler paInit;
  if (paInit.result() != paNoError)
    goto error;

  if (sine.open(Pa_GetDefaultOutputDevice())) {
    if (sine.start()) {
      printf("Play for %d seconds.\n", NUM_SECONDS);
      Pa_Sleep(NUM_SECONDS * 1000);

      sine.stop();
    }

    sine.close();
  }

  printf("Test finished.\n");
  return paNoError;

error:
  fprintf(stderr, "An error occurred while using the portaudio stream\n");
  fprintf(stderr, "Error number: %d\n", paInit.result());
  fprintf(stderr, "Error message: %s\n", Pa_GetErrorText(paInit.result()));
  return 1;
}