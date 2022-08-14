#include <iostream>

#include "state_manager.hpp"

StateManager *StateManager::instance_ = nullptr;

void StateManager::update_tempo(float tempo, const precise_time_t &timeref) {

  post(strand_, [this, tempo, timeref]() {
    tempo_ = tempo;
    time_ref_ = timeref;
  });
}

void StateManager::broadcast_tempo() {
  std::cout << "Broadcasting dddtempo " << tempo_ << " and time ref "
            << time_ref_.tv_sec << "." << time_ref_.tv_nsec << std::endl;
}

asio::strand<asio::any_io_executor> &StateManager::strand() { return strand_; }
