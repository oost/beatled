#include <iostream>

#include "state_manager.hpp"

StateManager::StateManager(asio::io_context &io_context)
    : strand_(asio::make_strand(io_context)) {}

void StateManager::update_tempo(float tempo, const precise_time_t &timeref) {
  asio::post(strand_, [this, tempo, timeref]() {
    tempo_ = tempo;
    time_ref_ = timeref;
  });
}

void StateManager::broadcast_tempo() {
  std::cout << "Broadcasting dddtempo " << tempo_ << " and time ref "
            << time_ref_.tv_sec << "." << time_ref_.tv_nsec << std::endl;
}

void StateManager::post_tempo(
    const std::function<void(float, uint64_t, uint32_t)> &post_cb) {
  asio::post(strand_, [&post_cb, this]() {
    post_cb(tempo_, time_ref_.tv_sec, time_ref_.tv_nsec);
  });
}

// asio::strand<asio::any_io_executor> &StateManager::strand() { return strand_;
// }
