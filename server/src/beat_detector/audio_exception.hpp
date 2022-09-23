#ifndef BEATDETECTOR_AUDIO_EXCEPTION_H
#define BEATDETECTOR_AUDIO_EXCEPTION_H

#include <exception>

#define TABLE_SIZE (200)

namespace beat_detector {

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

class AudioInputException : AudioException {
public:
  // std::runtime_error takes a const char* null-terminated string.
  // std::string_view may not be null-terminated, so it's not a good choice
  // here. Our ArrayException will take a const std::string& instead, which is
  // guaranteed to be null-terminated, and can be converted to a const char*.
  AudioInputException(const std::string &error)
      : AudioException{error.c_str()}
  // std::runtime_error will handle the string
  {}

  // no need to override what() since we can just use std::runtime_error::what()
};

class AudioFileException : AudioException {
public:
  // std::runtime_error takes a const char* null-terminated string.
  // std::string_view may not be null-terminated, so it's not a good choice
  // here. Our ArrayException will take a const std::string& instead, which is
  // guaranteed to be null-terminated, and can be converted to a const char*.
  AudioFileException(const std::string &error)
      : AudioException{error.c_str()}
  // std::runtime_error will handle the string
  {}

  // no need to override what() since we can just use std::runtime_error::what()
};

} // namespace beat_detector

#endif // BEATDETECTOR_AUDIO_EXCEPTION_H