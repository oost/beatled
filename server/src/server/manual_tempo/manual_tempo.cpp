#include <algorithm>
#include <spdlog/spdlog.h>

#include "core/clock.hpp"
#include "manual_tempo/manual_tempo.hpp"

namespace beatled::server {

using beatled::core::Clock;

// Plausible musical range. Below ~20 BPM the per-beat period gets long enough
// to feel broken; above ~400 BPM the visual is indistinguishable from strobe
// and the broadcast rate climbs needlessly.
static constexpr double kMinBpm = 20.0;
static constexpr double kMaxBpm = 400.0;

ManualTempo::ManualTempo(const std::string &id, asio::io_context &io_context,
                         StateManager &state_manager, next_beat_cb_t next_beat_callback)
    : ServiceControllerInterface{id}, state_manager_{state_manager},
      next_beat_callback_{std::move(next_beat_callback)}, strand_{asio::make_strand(io_context)},
      beat_timer_{strand_} {
  SPDLOG_INFO("Creating {}", name());
}

ManualTempo::~ManualTempo() {}

double ManualTempo::clamp_bpm(double bpm) {
  return std::clamp(bpm, kMinBpm, kMaxBpm);
}

void ManualTempo::start_sync() {
  // Re-arm from the strand so the timer state is only ever touched there.
  asio::post(strand_, [this]() {
    beat_count_ = 0;
    emit_beat();
  });
}

void ManualTempo::stop_sync() {
  asio::post(strand_, [this]() { beat_timer_.cancel(); });
}

void ManualTempo::emit_beat() {
  const double bpm = clamp_bpm(state_manager_.get_manual_bpm());
  const uint64_t period_us = static_cast<uint64_t>(60.0 * 1000000.0 / bpm);

  // Announce the upcoming beat one period ahead, in the same CLOCK_MONOTONIC
  // domain the TIME_SYNC handler reports (Clock::time_us_64) so controllers
  // schedule it against the offset they already negotiated.
  const uint64_t next_beat_time_ref = Clock::time_us_64() + period_us;
  ++beat_count_;

  SPDLOG_DEBUG("{} beat={} bpm={} next_ref={}", name(), beat_count_, bpm, next_beat_time_ref);
  if (next_beat_callback_) {
    // The count pairs with next_beat_time_ref: it names the beat that will
    // fire at that timestamp, not the one just emitted (see protocol.h).
    next_beat_callback_(next_beat_time_ref, bpm, bpm, beat_count_ + 1);
  }

  schedule_next_beat();
}

void ManualTempo::schedule_next_beat() {
  const double bpm = clamp_bpm(state_manager_.get_manual_bpm());
  const auto period = std::chrono::microseconds(static_cast<uint64_t>(60.0 * 1000000.0 / bpm));

  beat_timer_.expires_after(period);
  beat_timer_.async_wait([this](const std::error_code &ec) {
    if (ec == asio::error::operation_aborted) {
      return; // stopped or re-armed
    }
    if (ec) {
      SPDLOG_WARN("{} timer error: {}", name(), ec.message());
      return;
    }
    emit_beat();
  });
}

} // namespace beatled::server
