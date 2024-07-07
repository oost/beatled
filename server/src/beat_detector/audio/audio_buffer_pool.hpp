#ifndef BEAT_DETECTOR__AUDIO_BUFFER_POOL_HPP
#define BEAT_DETECTOR__AUDIO_BUFFER_POOL_HPP

#include <chrono>
#include <condition_variable>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <spdlog/spdlog.h>
#include <vector>

#include "audio_buffer.hpp"
#include "beat_detector/audio/config.h"

namespace beatled::detector {

/**
 * @brief Pool and queue of AudioBuffers
 * The `AudioBufferPool` class serves both as a pool of already instanciated
 * `AudioBuffer`s as well as a queue for `AudioBuffer`s to be passed from the
 * `AudioInput` to the processor (usually the `BeatDetector` instance)
 *
 * 1. The `AudioInput` will request an `AudioBuffer`
 * 2. The `AudioInput` will fill it in with new data and enqueue it in the queue
 * 3. The processor (e.g. `BeatDetector`) dequeues the buffer and processes it
 * 4. the processor releases the buffer back to the pool
 */
class AudioBufferPool {
public:
  /**
   * @brief Constructor
   * @param buffer_size max buffer size
   */
  AudioBufferPool(std::size_t buffer_size, double sample_rate)
      : buffer_size_{buffer_size}, sample_rate_{sample_rate}, active_{true} {
    // Add one buffer to pool
    add_buffer();
  }

  void set_sample_rate(double sample_rate) {
    sample_rate_ = sample_rate;
    std::unique_lock<std::mutex> L{pool_buffer_queue_mutex_};
    while (!pool_buffer_queue_.empty()) {
      total_pool_size_--;
      pool_buffer_queue_.pop();
    }
    add_buffer();
  }

  /**
   * @brief Size of the `AudioBuffer`s
   * @return
   */
  std::size_t buffer_size() const { return buffer_size_; }

  /**
   * @brief Indicates size of the pool
   * @return size of the pool
   */
  inline std::size_t pool_size() const {
    std::unique_lock<std::mutex> L{pool_buffer_queue_mutex_};
    return pool_buffer_queue_.size();
  }

  /**
   * @brief Indicates how many buffers are free
   * @return
   */
  inline std::size_t queue_size() const {
    std::lock_guard<std::mutex> L{filled_buffer_queue_mutex_};
    return filled_buffer_queue_.size();
  }

  /**
   * @brief Indicates full size of the pool
   * @return
   */
  inline std::size_t total_pool_size() const {
    std::unique_lock<std::mutex> L{pool_buffer_queue_mutex_};
    return total_pool_size_;
  }

  /**
   * @brief Gets a free buffer from pool or creates a new one
   * @return AudioBuffer::Ptr
   */
  AudioBuffer::Ptr get_new_buffer() {
    std::unique_lock<std::mutex> L{pool_buffer_queue_mutex_};
    if (pool_buffer_queue_.empty()) {
      total_pool_size_++;
      SPDLOG_INFO("Creating new AudioBuffer. Total pool size: {}",
                  total_pool_size_);

      return std::make_unique<AudioBuffer>(buffer_size_, sample_rate_);
    }

    // Get first element
    auto new_buffer = std::move(pool_buffer_queue_.front());

    // Remove first element from queue
    pool_buffer_queue_.pop();

    // Reset buffer
    new_buffer->reset_buffer();

    // Return buffer
    return std::move(new_buffer);
  }

  /**
   * @brief Returns a pointer to pool
   * @param buffer AudioBuffer to return
   */
  void enqueue(AudioBuffer::Ptr buffer) {
    std::lock_guard<std::mutex> L{filled_buffer_queue_mutex_};
    filled_buffer_queue_.push(std::move(buffer));

    // Tell the consumer it has an int
    filled_buffer_queue_cv_.notify_one();
  }

  void set_active(bool active) {
    std::lock_guard<std::mutex> L{filled_buffer_queue_mutex_};
    active_ = active;

    // Tell the consumer it has an int
    filled_buffer_queue_cv_.notify_one();
  }

  /**
   * @brief Returns a buffer, blocking
   * @return
   */
  AudioBuffer::Ptr dequeue_blocking() {
    std::unique_lock<std::mutex> filled_buffer_queue_lock{
        filled_buffer_queue_mutex_};
    filled_buffer_queue_cv_.wait(filled_buffer_queue_lock, [&]() {
      // Acquire the lock only if
      // the queue isn't empty
      return (!filled_buffer_queue_.empty()) || (!active_);
    });

    if (!active_) {
      return nullptr;
    }

    // We own the mutex here; pop the queue
    // until it empties out.
    auto val = std::move(filled_buffer_queue_.front());
    filled_buffer_queue_.pop();
    return val;
  }

  /**
   * @brief Indicates whether the queue is empty
   * @return
   */
  bool queue_empty() {
    std::unique_lock<std::mutex> filled_buffer_queue_lock{
        filled_buffer_queue_mutex_};
    return filled_buffer_queue_.empty();
  }

  /**
   * @brief Indicates whether the pool is empty
   * @return
   */
  bool pool_empty() {
    std::unique_lock<std::mutex> pool_buffer_queue_lock{
        pool_buffer_queue_mutex_};
    return pool_buffer_queue_.empty();
  }

  /**
   * @brief Returns a buffer to the free buffer queue
   * @param buffer
   */
  void release_buffer(AudioBuffer::Ptr buffer) {
    std::lock_guard<std::mutex> pool_buffer_queue_lock{
        pool_buffer_queue_mutex_};
    // TODO: add destroy buffer if enough buffers
    std::size_t pool_size_ = pool_buffer_queue_.size();
    if (pool_size_ > 3) {
      SPDLOG_INFO("Destroying AudioBuffer. Total pool size: {}", pool_size_);
      total_pool_size_--;
    } else {
      pool_buffer_queue_.push(std::move(buffer));
    }
  }

private:
  void add_buffer() {
    pool_buffer_queue_.push(
        std::move(std::make_unique<AudioBuffer>(buffer_size_, sample_rate_)));
    total_pool_size_++;
  }

  /**
   * @brief Condition variable for filled buffer queue
   */
  std::condition_variable filled_buffer_queue_cv_;

  /**
   * @brief Mutex for filled buffer queue
   */
  mutable std::mutex filled_buffer_queue_mutex_;

  /**
   * @brief Filled buffer queue
   */
  std::queue<std::unique_ptr<AudioBuffer>> filled_buffer_queue_;

  /**
   * @brief Mutex for pool buffer queue
   */
  mutable std::mutex pool_buffer_queue_mutex_;

  /**
   * @brief Pool buffer queue
   */
  std::queue<std::unique_ptr<AudioBuffer>> pool_buffer_queue_;

  /**
   * @brief Size of each individual buffer
   */
  std::size_t buffer_size_;

  /**
   * @brief Size of pool
   */
  std::size_t total_pool_size_ = 0;

  /**
   * @brief sample rate of the audio buffers
   * **/

  double sample_rate_;

  bool active_;
};

} // namespace beatled::detector

#endif // BEAT_DETECTOR__AUDIO_BUFFER_POOL_HPP