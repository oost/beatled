#ifndef BEAT_DETECTOR__AUDIO_BUFFER_HPP
#define BEAT_DETECTOR__AUDIO_BUFFER_HPP

#include <chrono>
#include <condition_variable>
#include <ctime>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <spdlog/spdlog.h>
#include <vector>

#include "beat_detector/audio/config.h"

namespace beatled::detector {

/**
 * @brief Audio Buffer
 * Audio Buffer wrapping a vector to audio_buffer_t (defaults to double).
 */
class AudioBuffer {
public:
  using Ptr = std::unique_ptr<AudioBuffer>;
  using audio_buffer_data_t = std::vector<audio_buffer_t>;

  /**
   * @brief Constructor
   * @param buffer_size Size of audio buffer
   */
  AudioBuffer(std::size_t buffer_size, double sample_rate)
      : sample_rate_{sample_rate} {
    data_.reserve(buffer_size);
  }

  /**
   * @brief Destructor
   */
  virtual ~AudioBuffer() { SPDLOG_INFO("Destroying AudioBuffer"); }

  /**
   * @brief Start time at which the buffer was captured
   * @return
   */
  uint64_t start_time() const { return buffer_start_time_; }

  /**
   * @brief Time at the middle of the buffer
   * @return
   */
  uint64_t mid_time() const {
    return buffer_start_time_ + data_.size() / 2 * 1000000 / sample_rate_;
  }

  /**
   * @brief Set the start time at which the audio buffer was captured
   * @param buffer_start_time
   */
  void set_start_time(uint64_t buffer_start_time) {
    buffer_start_time_ = buffer_start_time;
  }

  /**
   * @brief Reset buffer to 0
   */
  void reset_buffer() {
    data_.resize(0);
    buffer_start_time_ = 0;
  }

  /**
   * @brief Length of the buffer
   * @return
   */
  inline size_t size() const { return data_.size(); }

  /**
   * @briefmIndicates whether the underlying vector is at capacity
   * @return
   */
  inline bool is_full() const { return data_.size() == data_.capacity(); }

  /**
   * @brief Copy data from input_buffer to internal vector
   * @param input_buffer Pointer to input_buffer
   * @param input_buffer_size Size of pointer
   * @return Count of values copied
   */
  int copy_raw_data(float *input_buffer, std::size_t input_buffer_size) {
    int remaining_capacity = data_.capacity() - data_.size();
    int items_to_copy = (remaining_capacity > input_buffer_size)
                            ? input_buffer_size
                            : remaining_capacity;

    for (int i = 0; i < items_to_copy; i++) {
      data_.push_back(input_buffer[i]);
    }
    return items_to_copy;
  }

  /**
   * @brief Pointer to underlying data
   * @return Raw pointer to data array
   */
  inline const audio_buffer_data_t &data() const { return data_; }

private:
  AudioBuffer &operator=(const AudioBuffer &) = delete;
  AudioBuffer(const AudioBuffer &) = delete;

  /**
   * @brief Underlying data vector
   */
  audio_buffer_data_t data_;

  /**
   * @brief Start time of buffer
   */
  uint64_t buffer_start_time_{0};

  double sample_rate_;
};

} // namespace beatled::detector

#endif // BEAT_DETECTOR__AUDIO_BUFFER_HPP