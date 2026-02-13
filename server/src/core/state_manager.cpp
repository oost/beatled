#include <cstring>
#include <spdlog/spdlog.h>

#include "core/state_manager.hpp"

namespace beatled::core {

StateManager::StateManager() {}

void StateManager::update_tempo(float tempo, uint64_t timeref) {
  std::unique_lock lk(tempo_mtx_);
  tempo_ = tempo;
  time_ref_ = timeref;
}

tempo_ref_t StateManager::get_tempo_ref() const {
  std::unique_lock lk(tempo_mtx_);
  tempo_ref_t tr = {
      .beat_time_ref = time_ref_,
      .tempo_period_us = tempo_ > 0.0f
                             ? static_cast<uint32_t>(60 * 1000000UL / tempo_)
                             : 0,
      .tempo = tempo_};
  return tr;
}

void StateManager::update_program_id(uint16_t program_id) {
  program_id_ = program_id;
}
uint16_t StateManager::get_program_id() const { return program_id_; }

void StateManager::update_next_beat(uint64_t next_beat_time_ref) {
  next_beat_time_ref_ = next_beat_time_ref;
  for (auto &cb : on_next_beat_cbs_) {
    cb(next_beat_time_ref_);
  }
}

uint64_t StateManager::get_next_beat_time_ref() const {
  return next_beat_time_ref_;
}

ClientStatus::Ptr
StateManager::client_status(const ClientStatus::board_id_t &board_id) const {
  std::unique_lock lk(client_mtx_);

  for (auto &el : clients_) {
    if (el->board_id == board_id) {
      return el;
    }
  }
  return nullptr;
}

ClientStatus::Ptr
StateManager::client_status(const asio::ip::address &ip_address) const {
  std::unique_lock lk(client_mtx_);

  for (auto &el : clients_) {
    if (el->ip_address == ip_address) {
      return el;
    }
  }
  return nullptr;
}

void StateManager::register_client(ClientStatus::Ptr client_status) {
  std::unique_lock lk(client_mtx_);

  for (auto it = clients_.begin(); it != clients_.end();) {
    if (client_status->board_id == (*it)->board_id) {
      // board_id has already been registered. Delete it
      SPDLOG_INFO("Board was already registered. Deleting old registration");
      it = clients_.erase(it);
      continue;
    }
    if (client_status->ip_address == (*it)->ip_address) {
      SPDLOG_INFO("IP was registered with another board");
      it = clients_.erase(it);
      continue;
    }
    it++;
  }

  clients_.push_back(client_status);
}

ClientStatus::client_map_t StateManager::get_clients() const {
  std::unique_lock lk(client_mtx_);
  return clients_;
}

} // namespace beatled::core
