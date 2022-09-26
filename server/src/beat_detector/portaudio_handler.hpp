#ifndef BEATDETECTOR_PORTAUDIOHANDLER_H
#define BEATDETECTOR_PORTAUDIOHANDLER_H

#include "audio_exception.hpp"
#include "portaudio.h"

namespace beat_detector {

class ScopedPaHandler {
public:
  ScopedPaHandler() : _result(Pa_Initialize()) {
    if (_result != paNoError) {
      throw AudioInputException("Couldn't start pa handler.");
    }
  }

  ~ScopedPaHandler() {
    if (_result == paNoError) {
      Pa_Terminate();
    }
  }

  PaError result() const { return _result; }

private:
  PaError _result;
};
} // namespace beat_detector

#endif // BEATDETECTOR_PORTAUDIOHANDLER_H