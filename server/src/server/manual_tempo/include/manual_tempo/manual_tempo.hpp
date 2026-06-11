#ifndef SERVER_MANUAL_TEMPO_H
#define SERVER_MANUAL_TEMPO_H

#include <asio.hpp>
#include <atomic>
#include <chrono>
#include <functional>

#include "core/interfaces/service_controller.hpp"
#include "core/state_manager.hpp"

using beatled::core::ServiceControllerInterface;
using beatled::core::StateManager;

namespace beatled::server {

// Software metronome that stands in for the audio BeatDetector when it is
// off. It emits evenly-spaced "next beat" events at the operator-chosen BPM
// (read from StateManager::get_manual_bpm()) through the same callback the
// detector uses, so the downstream path (update_tempo / update_next_beat /
// TempoBroadcaster::broadcast_next_beat) is identical regardless of where the
// beat came from.
//
// The two tempo sources are mutually exclusive; the HTTP service-control
// handler stops one when the other starts.
class ManualTempo : public ServiceControllerInterface {
public:
  // Matches BeatDetector::beat_detector_cb_t: (next_beat_time_ref, tempo,
  // estimated_tempo, beat_count).
  using next_beat_cb_t = std::function<void(uint64_t, double, double, uint32_t)>;

  ManualTempo(const std::string &id, asio::io_context &io_context, StateManager &state_manager,
              next_beat_cb_t next_beat_callback);
  ~ManualTempo();

  void start_sync() override;
  void stop_sync() override;

private:
  const char *SERVICE_NAME = "Manual Tempo";
  const char *service_name() const override { return SERVICE_NAME; }

  // Emit one beat and re-arm the timer for the next one.
  void schedule_next_beat();
  void emit_beat();

  // Clamp the configured BPM into a sane range so a typo can't wedge the
  // timer at 0 us (busy loop) or an absurd period.
  static double clamp_bpm(double bpm);

  StateManager &state_manager_;
  next_beat_cb_t next_beat_callback_;

  asio::strand<asio::io_context::executor_type> strand_;
  asio::high_resolution_timer beat_timer_;
  uint32_t beat_count_{0};
};

} // namespace beatled::server

#endif // SERVER_MANUAL_TEMPO_H
