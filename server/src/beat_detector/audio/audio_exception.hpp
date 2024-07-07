#ifndef BEATDETECTOR_AUDIO_EXCEPTION_H
#define BEATDETECTOR_AUDIO_EXCEPTION_H

#include <exception>

namespace beatled::detector {

/**
 * @brief Exception for Audio classes
 */
class AudioException : public std::runtime_error {
public:
  // std::runtime_error takes a const char* null-terminated string.
  // std::string_view may not be null-terminated, so it's not a good choice
  // here. Our ArrayException will take a const std::string& instead, which is
  // guaranteed to be null-terminated, and can be converted to a const char*.
  AudioException(const std::string &error)
      : std::runtime_error{error.c_str()}
  // std::runtime_error will handle the string
  {}

  // no need to override what() since we can just use std::runtime_error::what()
};

/**
 * @brief Exception for AudioInput
 */
class AudioInputException : AudioException {
public:
  AudioInputException(const std::string &error)
      : AudioException{error.c_str()} {}
};

/**
 * @brief Exception for AudioFile
 */
class AudioFileException : AudioException {
public:
  AudioFileException(const std::string &error)
      : AudioException{error.c_str()} {}
};

} // namespace beatled::detector

#endif // BEATDETECTOR_AUDIO_EXCEPTION_H