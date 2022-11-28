#include <cstring>
#include <iostream>
#include <spdlog/spdlog.h>

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

ClientStatus *
StateManager::client_status(const asio::ip::address &client_address) const {
  if (auto search = clients_.find(client_address); search != clients_.end()) {
    return (search->second).get();
  }
  return nullptr;
}

ClientStatus *
StateManager::register_client(const asio::ip::address &new_client_address,
                              const ClientStatus::board_id_t &new_board_id) {
  for (const auto &it : clients_) {
    if (new_board_id == it.second->board_id) {
      // board_id has already been registered. Delete it
      SPDLOG_INFO("Board was already registered. Deleting old registration");
      clients_.erase(it.first);
      break;
    }
    if (new_client_address == it.first) {
      return nullptr;
    }
  }

  ClientStatus::Ptr ptr = std::make_unique<ClientStatus>();

  ptr->client_id = ++client_id_max_;
  std::copy(std::begin(new_board_id), std::end(new_board_id),
            std::begin(ptr->board_id));

  ClientStatus *ret = ptr.get();
  clients_[new_client_address] = std::move(ptr);

  return ret;
}
