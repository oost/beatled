#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include <asio.hpp>
#include <map>
#include <memory>
#include <mutex>
#include <time.h>

#include "client_status.hpp"

typedef struct tempo_ref {
  uint64_t beat_time_ref;
  uint32_t tempo_period_us;
  float tempo;
} tempo_ref_t;

class StateManager {
public:
  using Ptr = std::shared_ptr<StateManager>;
  StateManager();

  void update_tempo(float tempo, uint64_t timeref);
  tempo_ref_t get_tempo_ref();

  ClientStatus *client_status(const asio::ip::address &client_address) const;
  ClientStatus *register_client(const asio::ip::address &client_address,
                                const ClientStatus::board_id_t &new_board_id);

private:
  StateManager(const StateManager &) = delete;
  StateManager &operator=(const StateManager &) = delete;

  uint16_t client_id_max_ = 0;
  float tempo_;
  uint64_t time_ref_;
  std::mutex mtx_;
  client_map_t clients_;
};

#endif // STATE_MACHINE_H