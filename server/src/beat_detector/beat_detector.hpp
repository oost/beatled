#ifndef BEAT_DETECTOR_BEAT_DETECTOR_H
#define BEAT_DETECTOR_BEAT_DETECTOR_H

#include <asio.hpp>
#include <memory>

#include "audio_buffer_pool.hpp"

namespace beat_detector {
class BeatDetector {
public:
  BeatDetector(uint32_t sample_rate);

  void run();
  void request_stop();

private:
  void do_detect_tempo();

  std::thread thread_;
  std::future<void> bd_thread_future_;
  std::atomic_bool stop_requested_;

  uint32_t sample_rate_;
};

} // namespace beat_detector
#endif // BEAT_DETECTOR_BEAT_DETECTOR_H