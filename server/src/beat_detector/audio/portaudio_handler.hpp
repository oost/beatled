

#ifndef BEATDETECTOR_PORTAUDIOHANDLER_H
#define BEATDETECTOR_PORTAUDIOHANDLER_H

#include <portaudio.h>
#include <spdlog/spdlog.h>

#include "audio_exception.hpp"

namespace beatled::detector {

class PortaudioHandle {
public:
  PortaudioHandle() : _result(Pa_Initialize()) {
    if (_result != paNoError) {
      throw AudioInputException("Couldn't start pa handler.");
    }
    SPDLOG_INFO("Initialized PortAudio");
  }

  ~PortaudioHandle() {
    if (_result == paNoError) {
      Pa_Terminate();
      SPDLOG_INFO("Terminating PortAudio");
    }
  }

  PaError result() const { return _result; }

private:
  PaError _result;
};
} // namespace beatled::detector

#endif // BEATDETECTOR_PORTAUDIOHANDLER_H
