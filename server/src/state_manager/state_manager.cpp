#include <iostream>

#include "state_manager/state_manager.hpp"

StateManager::StateManager() {}

void StateManager::update_tempo(float tempo, uint64_t timeref) {
  std::unique_lock lk(mtx_);
  tempo_ = tempo;
  time_ref_ = timeref;
}

tempo_ref_t StateManager::get_tempo_ref() {
  std::unique_lock lk(mtx_);
  tempo_ref_t tr = {.beat_time_ref = time_ref_,
                    .tempo_period_us =
                        static_cast<uint32_t>(60 * 1000000UL / tempo_),
                    .tempo = tempo_};
  return tr;
}
