#ifndef AUDIO_BUFFER_POOL_H
#define AUDIO_BUFFER_POOL_H

#include <chrono>
#include <condition_variable>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <vector>

#include "../config.hpp"

namespace beat_detector {
class AudioBuffer;

using AudioBuffer_ptr = std::unique_ptr<AudioBuffer>;
using start_time_t = std::chrono::time_point<std::chrono::system_clock>;
using audio_buffer_data_t = std::vector<audio_buffer_t>;

class AudioBuffer {
public:
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

  const audio_buffer_t *raw_data() const { return data_.data(); }

  inline const audio_buffer_data_t &data() const { return data_; }
  // ~AudioBuffer() { std::cerr << "Destroying AudioBuffer..." << std::endl; }

private:
  AudioBuffer &operator=(const AudioBuffer &) = delete;
  AudioBuffer(const AudioBuffer &) = delete;

  audio_buffer_data_t data_;
  std::size_t data_size_{0};
  start_time_t buffer_start_time_;
};

class AudioBufferPool {
public:
  AudioBufferPool(std::size_t buffer_size) : buffer_size_{buffer_size} {
    pool_buffer_queue_.push(
        std::move(std::make_unique<AudioBuffer>(buffer_size_)));
  }

  size_t pool_size() const { return pool_buffer_queue_.size(); }
  size_t queue_size() const { return filled_buffer_queue_.size(); }

  AudioBuffer_ptr get_new_buffer() {
    std::unique_lock<std::mutex> L{pool_buffer_queue_mutex_};
    if (pool_buffer_queue_.empty()) {
      std::cout << "Creating new AudioBuffer" << std::endl;
      return std::make_unique<AudioBuffer>(buffer_size_);
    }
    auto new_buffer = std::move(pool_buffer_queue_.front());
    new_buffer->reset_buffer();
    pool_buffer_queue_.pop();

    return std::move(new_buffer);
  }

  void enqueue(AudioBuffer_ptr buffer) {
    std::lock_guard<std::mutex> L{filled_buffer_queue_mutex_};
    filled_buffer_queue_.push(std::move(buffer));

    // Tell the consumer it has an int
    filled_buffer_queue_cv_.notify_one();
  }

  AudioBuffer_ptr dequeue_blocking() {
    std::unique_lock<std::mutex> L{filled_buffer_queue_mutex_};
    filled_buffer_queue_cv_.wait(L, [&]() {
      // Acquire the lock only if
      // the queue
      // isn't empty
      return !filled_buffer_queue_.empty();
    });

    // We own the mutex here; pop the queue
    // until it empties out.
    auto val = std::move(filled_buffer_queue_.front());
    filled_buffer_queue_.pop();
    return val;
  }

  bool queue_empty() {
    std::unique_lock<std::mutex> L{filled_buffer_queue_mutex_};
    return filled_buffer_queue_.empty();
  }

  bool pool_empty() {
    std::unique_lock<std::mutex> L{pool_buffer_queue_mutex_};
    return pool_buffer_queue_.empty();
  }

  void release_buffer(AudioBuffer_ptr buffer) {
    std::lock_guard<std::mutex> L{pool_buffer_queue_mutex_};
    pool_buffer_queue_.push(std::move(buffer));
  }

private:
  std::condition_variable filled_buffer_queue_cv_;
  std::mutex filled_buffer_queue_mutex_;
  std::queue<std::unique_ptr<AudioBuffer>> filled_buffer_queue_;

  std::mutex pool_buffer_queue_mutex_;
  std::queue<std::unique_ptr<AudioBuffer>> pool_buffer_queue_;

  std::size_t buffer_size_;
};

} // namespace beat_detector

#endif // AUDIO_BUFFER_POOL_H