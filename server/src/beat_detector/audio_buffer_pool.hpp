#ifndef AUDIO_BUFFER_POOL_H
#define AUDIO_BUFFER_POOL_H

#include <array>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>

#include "../config.h"

namespace beat_detector {

class AudioBuffer {
public:
  AudioBuffer() {}

  void setStartTime(
      std::chrono::time_point<std::chrono::system_clock> &&buffer_start_time) {
    buffer_start_time_ = std::move(buffer_start_time);
  }

  audio_buffer_t *raw_data() { return data_.data(); }

private:
  AudioBuffer &operator=(const AudioBuffer &) = delete;
  AudioBuffer(const AudioBuffer &) = delete;
  std::array<audio_buffer_t, constants::BUFFER_SIZE> data_;
  std::chrono::time_point<std::chrono::system_clock> buffer_start_time_;
};

class AudioBufferPool {
public:
  AudioBufferPool() {}

  std::unique_ptr<AudioBuffer> get_new_buffer() {
    std::unique_lock<std::mutex> L{pool_buffer_queue_mutex_};
    if (pool_buffer_queue_.empty()) {
      return std::make_unique<AudioBuffer>();
    }
    auto new_buffer = std::move(pool_buffer_queue_.front());
    pool_buffer_queue_.pop();

    return new_buffer;
  }

  void enqueue_buffer(std::unique_ptr<AudioBuffer> buffer) {
    std::lock_guard<std::mutex> L{filled_buffer_queue_mutex_};
    filled_buffer_queue_.push(std::move(buffer));

    // Tell the consumer it has an int
    filled_buffer_queue_cv_.notify_one();
  }

  std::unique_ptr<AudioBuffer> dequeue_buffer_blocking() {
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

  void release_buffer(std::unique_ptr<AudioBuffer> buffer) {
    std::lock_guard<std::mutex> L{pool_buffer_queue_mutex_};
    pool_buffer_queue_.push(std::move(buffer));
  }

private:
  std::condition_variable filled_buffer_queue_cv_;
  std::mutex filled_buffer_queue_mutex_;
  std::queue<std::unique_ptr<AudioBuffer>> filled_buffer_queue_;

  std::mutex pool_buffer_queue_mutex_;
  std::queue<std::unique_ptr<AudioBuffer>> pool_buffer_queue_;
};

} // namespace beat_detector

#endif // AUDIO_BUFFER_POOL_H