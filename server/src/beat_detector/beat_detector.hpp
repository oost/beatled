#ifndef BEAT_DETECTOR_BEAT_DETECTOR_H
#define BEAT_DETECTOR_BEAT_DETECTOR_H

#include <asio.hpp>
#include <memory>

#include "audio_buffer_pool.hpp"

namespace beat_detector {
class BeatDetector {
public:
  BeatDetector();

  void run();

private:
  void do_detect_tempo();

  /// The io_context used to perform asynchronous operations.
  asio::io_context io_context_;

  /// The signal_set is used to register for process termination notifications.
  asio::signal_set signals_;

  asio::thread *thread_;
  asio::high_resolution_timer timer_;
  std::chrono::nanoseconds alarm_period_;

  AudioBufferPool audio_buffer_pool_;
};

} // namespace beat_detector
#endif // BEAT_DETECTOR_BEAT_DETECTOR_H