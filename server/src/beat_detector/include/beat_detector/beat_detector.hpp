#ifndef BEAT_DETECTOR_BEAT_DETECTOR_H
#define BEAT_DETECTOR_BEAT_DETECTOR_H

#include <asio.hpp>
#include <atomic>

#include "state_manager/state_manager.hpp"

namespace beat_detector {
class BeatDetector {
public:
  BeatDetector(StateManager &state_manager, uint32_t sample_rate);

  void run();
  void request_stop();
  void stop_blocking();
  bool is_running();

private:
  void do_detect_tempo();

  std::future<void> bd_thread_future_;
  std::atomic_bool stop_requested_;
  std::atomic_bool is_running_ = false;

  uint32_t sample_rate_;
  StateManager &state_manager_;
};

} // namespace beat_detector
#endif // BEAT_DETECTOR_BEAT_DETECTOR_H