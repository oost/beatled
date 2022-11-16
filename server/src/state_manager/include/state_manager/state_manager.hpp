#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include <memory>
#include <mutex>
#include <time.h>

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

private:
  StateManager(const StateManager &) = delete;
  StateManager &operator=(const StateManager &) = delete;
  float tempo_;
  uint64_t time_ref_;

  std::mutex mtx_;
};

#endif // STATE_MACHINE_H