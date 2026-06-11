#ifndef CORE_STATE_MANAGER_HPP
#define CORE_STATE_MANAGER_HPP

#include <asio.hpp>
#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <time.h>

#include "client_status.hpp"
#include "clock.hpp"

namespace beatled::core {

static constexpr uint64_t DEVICE_EXPIRY_US = 30 * 1000000ULL; // 30 seconds

typedef struct tempo_ref {
  uint64_t beat_time_ref;
  uint32_t tempo_period_us;
  float tempo;
} tempo_ref_t;

class StateManager {
public:
  using Ptr = std::shared_ptr<StateManager>;
  using on_next_beat_cb_t = std::function<void(uint64_t)>;

  StateManager();

  void update_tempo(float tempo, uint64_t timeref);
  tempo_ref_t get_tempo_ref() const;

  void update_program_id(uint16_t program_id);
  uint16_t get_program_id() const;

  void update_next_beat(uint64_t next_beat_time_ref);
  uint64_t get_next_beat_time_ref() const;

  // Operator-chosen BPM used by the ManualTempo service when the audio beat
  // detector is off. Stored here (rather than on the service) so the HTTP
  // handler can set it without downcasting, and so it survives the service
  // being toggled off and back on. Clamped by the API layer.
  void set_manual_bpm(float bpm) { manual_bpm_ = bpm; }
  float get_manual_bpm() const { return manual_bpm_; }

  ClientStatus::Ptr client_status(const ClientStatus::board_id_t &board_id) const;
  ClientStatus::Ptr client_status(const asio::ip::address &ip_address) const;
  void register_client(ClientStatus::Ptr client_status);
  ClientStatus::client_map_t get_clients();

  // Record a fresh one-way-delay sample for the given client. Smoothed
  // server-side with an EWMA so a single jittery sample doesn't snap the
  // broadcaster's compensation around.
  void update_client_owd(const asio::ip::address &ip_address, uint64_t owd_us);

  using on_program_change_cb_t = std::function<void(uint16_t)>;

  // Must be called during construction only (before threads start).
  void register_next_beat_cb(const on_next_beat_cb_t &cb) { on_next_beat_cbs_.push_back(cb); }
  void register_program_change_cb(const on_program_change_cb_t &cb) {
    on_program_change_cbs_.push_back(cb);
  }

private:
  StateManager(const StateManager &) = delete;
  StateManager &operator=(const StateManager &) = delete;

  float tempo_ = 0.0f;
  uint64_t time_ref_ = 0;
  std::atomic<uint64_t> next_beat_time_ref_{0};
  std::atomic<uint16_t> program_id_{0};
  std::atomic<float> manual_bpm_{120.0f};
  mutable std::mutex tempo_mtx_;
  mutable std::mutex client_mtx_;
  ClientStatus::client_map_t clients_;
  std::vector<on_next_beat_cb_t> on_next_beat_cbs_;
  std::vector<on_program_change_cb_t> on_program_change_cbs_;
};

} // namespace beatled::core

#endif // CORE_STATE_MANAGER_HPP