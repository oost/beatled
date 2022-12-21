#ifndef BEAT_DETECTOR_BEAT_DETECTOR_H
#define BEAT_DETECTOR_BEAT_DETECTOR_H

#include <asio.hpp>
#include <atomic>

#include "core/interfaces/service_controller.hpp"
#include "core/state_manager.hpp"

namespace beat_detector {
class BeatDetector : public ServiceControllerInterface {
public:
  BeatDetector(const std::string &id, StateManager &state_manager,
               uint32_t sample_rate);

  void start_sync() override;
  void stop_sync() override;
  void stop_blocking();

private:
  const char *SERVICE_NAME = "Beat Detector";
  void do_detect_tempo();

  const char *service_name() const override { return SERVICE_NAME; }

  std::future<void> bd_thread_future_;
  std::atomic_bool stop_requested_;
  std::atomic_bool is_running_ = false;

  uint32_t sample_rate_;
  StateManager &state_manager_;
};

} // namespace beat_detector
#endif // BEAT_DETECTOR_BEAT_DETECTOR_H