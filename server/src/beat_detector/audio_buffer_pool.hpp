#ifndef BEAT_DETECTOR__AUDIO_BUFFER_POOL_HPP
#define BEAT_DETECTOR__AUDIO_BUFFER_POOL_HPP

#include <chrono>
#include <condition_variable>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <vector>

#include "../config.hpp"
#include "audio_buffer.hpp"

namespace beat_detector {

class AudioBufferPool {
public:
  AudioBufferPool(std::size_t buffer_size) : buffer_size_{buffer_size} {
    pool_buffer_queue_.push(
        std::move(std::make_unique<AudioBuffer>(buffer_size_)));
    total_pool_size_++;
  }

  // TODO: Adding mutex not possible on const method?
  inline size_t pool_size() const {
    std::unique_lock<std::mutex> L{pool_buffer_queue_mutex_};
    return pool_buffer_queue_.size();
  }
  inline size_t queue_size() const {
    std::lock_guard<std::mutex> L{filled_buffer_queue_mutex_};
    return filled_buffer_queue_.size();
  }
  inline size_t total_pool_size() const {
    std::unique_lock<std::mutex> L{pool_buffer_queue_mutex_};
    return total_pool_size_;
  }

  AudioBuffer::Ptr get_new_buffer() {
    std::unique_lock<std::mutex> L{pool_buffer_queue_mutex_};
    if (pool_buffer_queue_.empty()) {
      total_pool_size_++;
      std::cout << "Creating new AudioBuffer. Total pool size: "
                << total_pool_size_ << std::endl;

      return std::make_unique<AudioBuffer>(buffer_size_);
    }
    auto new_buffer = std::move(pool_buffer_queue_.front());
    new_buffer->reset_buffer();
    pool_buffer_queue_.pop();

    return std::move(new_buffer);
  }

  void enqueue(AudioBuffer::Ptr buffer) {
    std::lock_guard<std::mutex> L{filled_buffer_queue_mutex_};
    filled_buffer_queue_.push(std::move(buffer));

    // Tell the consumer it has an int
    filled_buffer_queue_cv_.notify_one();
  }

  AudioBuffer::Ptr dequeue_blocking() {
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

  void release_buffer(AudioBuffer::Ptr buffer) {
    std::lock_guard<std::mutex> L{pool_buffer_queue_mutex_};
    pool_buffer_queue_.push(std::move(buffer));
  }

private:
  std::condition_variable filled_buffer_queue_cv_;
  mutable std::mutex filled_buffer_queue_mutex_;
  std::queue<std::unique_ptr<AudioBuffer>> filled_buffer_queue_;

  mutable std::mutex pool_buffer_queue_mutex_;
  std::queue<std::unique_ptr<AudioBuffer>> pool_buffer_queue_;

  std::size_t buffer_size_;
  std::size_t total_pool_size_ = 0;
};

} // namespace beat_detector

#endif // BEAT_DETECTOR__AUDIO_BUFFER_POOL_HPP