#ifndef BEAT_DETECTOR__AUDIO_BUFFER_HPP
#define BEAT_DETECTOR__AUDIO_BUFFER_HPP

#include <chrono>
#include <condition_variable>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <vector>

#include "../config.hpp"

namespace beat_detector {

using start_time_t = std::chrono::time_point<std::chrono::system_clock>;
using audio_buffer_data_t = std::vector<audio_buffer_t>;

class AudioBuffer {
public:
  using Ptr = std::unique_ptr<AudioBuffer>;

  AudioBuffer(std::size_t buffer_size) { data_.reserve(buffer_size); }

  start_time_t start_time() const { return buffer_start_time_; }
  void set_start_time(const start_time_t &buffer_start_time) {
    buffer_start_time_ = buffer_start_time;
  }

  void reset_buffer() {
    data_.resize(0);
    buffer_start_time_ = start_time_t();
  }
  inline size_t size() const { return data_.size(); }

  inline bool is_full() const { return data_.size() == data_.capacity(); }

  int copy_raw_data(float *input_buffer, size_t input_buffer_size) {
    int remaining_capacity = data_.capacity() - data_.size();
    int items_to_copy = (remaining_capacity > input_buffer_size)
                            ? input_buffer_size
                            : remaining_capacity;

    for (int i = 0; i < items_to_copy; i++) {
      data_.push_back(input_buffer[i]);
    }
    return items_to_copy;
  }

  inline const audio_buffer_data_t &data() const { return data_; }

private:
  AudioBuffer &operator=(const AudioBuffer &) = delete;
  AudioBuffer(const AudioBuffer &) = delete;

  audio_buffer_data_t data_;
  std::size_t data_size_{0};
  start_time_t buffer_start_time_;
};

} // namespace beat_detector

#endif // BEAT_DETECTOR__AUDIO_BUFFER_HPP