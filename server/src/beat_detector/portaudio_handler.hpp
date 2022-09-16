#ifndef BEATDETECTOR_PORTAUDIOHANDLER_H
#define BEATDETECTOR_PORTAUDIOHANDLER_H

#include "portaudio.h"

class ScopedPaHandler {
public:
  ScopedPaHandler() : _result(Pa_Initialize()) {}

  ~ScopedPaHandler() {
    if (_result == paNoError) {
      Pa_Terminate();
    }
  }

  PaError result() const { return _result; }

private:
  PaError _result;
};

#endif // BEATDETECTOR_PORTAUDIOHANDLER_H