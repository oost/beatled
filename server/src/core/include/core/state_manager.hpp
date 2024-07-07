#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include <asio.hpp>
#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <time.h>

#include "client_status.hpp"

namespace beatled::core {

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

  ClientStatus::Ptr
  client_status(const ClientStatus::board_id_t &board_id) const;
  ClientStatus::Ptr client_status(const asio::ip::address &ip_address) const;
  void register_client(ClientStatus::Ptr client_status);

  void register_next_beat_cb(const on_next_beat_cb_t &cb) {
    on_next_beat_cbs_.push_back(cb);
  }

private:
  StateManager(const StateManager &) = delete;
  StateManager &operator=(const StateManager &) = delete;

  uint16_t client_id_max_ = 0;
  float tempo_;
  uint64_t time_ref_;
  uint64_t next_beat_time_ref_;
  std::atomic<uint16_t> program_id_;
  mutable std::mutex tempo_mtx_;
  mutable std::mutex client_mtx_;
  ClientStatus::client_map_t clients_;
  std::vector<on_next_beat_cb_t> on_next_beat_cbs_;
};

} // namespace beatled::core

#endif // STATE_MACHINE_H