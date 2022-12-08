#ifndef BEATDETECTOR_PORTAUDIOHANDLER_H
#define BEATDETECTOR_PORTAUDIOHANDLER_H

#include <portaudio.h>
#include <spdlog/spdlog.h>

#include "audio_exception.hpp"

namespace beat_detector {

class ScopedPaHandler {
public:
  ScopedPaHandler() : _result(Pa_Initialize()) {
    if (_result != paNoError) {
      throw AudioInputException("Couldn't start pa handler.");
    }
    SPDLOG_INFO("Initialized PortAudio");
  }

  ~ScopedPaHandler() {
    if (_result == paNoError) {
      Pa_Terminate();
      SPDLOG_INFO("Terminating PortAudio");
    }
  }

  PaError result() const { return _result; }

private:
  PaError _result;
};
} // namespace beat_detector

#endif // BEATDETECTOR_PORTAUDIOHANDLER_H