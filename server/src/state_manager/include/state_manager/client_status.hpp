#ifndef STATE_MANAGER__CLIENT_STATUS_H
#define STATE_MANAGER__CLIENT_STATUS_H

#include <asio.hpp>
#include <cstdint>

#include "beatled/protocol.h"

class ClientStatus {
public:
  using Ptr = std::unique_ptr<ClientStatus>;
  using board_id_t = std::array<char, 2 * PICO_UNIQUE_BOARD_ID_SIZE_BYTES + 1>;

  uint16_t client_id;
  uint64_t last_status_time;
  board_id_t board_id;
};

using client_map_t = std::map<asio::ip::address, ClientStatus::Ptr>;

#endif // STATE_MANAGER__CLIENT_STATUS_H