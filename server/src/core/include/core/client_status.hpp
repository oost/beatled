#ifndef STATE_MANAGER__CLIENT_STATUS_H
#define STATE_MANAGER__CLIENT_STATUS_H

#include <algorithm> // std::copy
#include <array>
#include <asio.hpp>
#include <cstdint>
#include <fmt/ranges.h>
#include <forward_list>
#include <functional>
#include <iomanip>  // std::setw, std::setfill
#include <sstream>  // std::ostringstream
#include <nlohmann/json.hpp>

#include "beatled/protocol.h"

namespace beatled::core {

using json = nlohmann::json;

class ClientStatus {
public:
  using Ptr = std::shared_ptr<ClientStatus>;
  using board_id_t = std::array<char, 2 * PICO_UNIQUE_BOARD_ID_SIZE_BYTES + 1>;
  using client_map_t = std::vector<ClientStatus::Ptr>;

  ClientStatus(const board_id_t &new_board_id, const asio::ip::address &new_ip_address)
      : board_id{new_board_id}, ip_address{new_ip_address} {}

  ClientStatus(const unsigned char new_board_id[], const asio::ip::address &new_ip_address)
      : ip_address{new_ip_address} {
    for (int i = 0; i < 2 * PICO_UNIQUE_BOARD_ID_SIZE_BYTES + 1; i++) {
      board_id[i] = new_board_id[i];
    }
  }

  uint16_t client_id;
  uint64_t last_status_time;
  board_id_t board_id;
  asio::ip::address ip_address;
};

// Manually define to_json for ClientStatus to have full control over board_id serialization
// board_id contains raw binary bytes from the Pico, so we convert to hex string
inline void to_json(json &j, const ClientStatus &cs) {
  // Convert raw binary board_id bytes to hex string representation
  // Always convert exactly 8 bytes (PICO_UNIQUE_BOARD_ID_SIZE_BYTES)
  std::ostringstream hex_stream;
  hex_stream << std::hex << std::setfill('0');

  for (size_t i = 0; i < PICO_UNIQUE_BOARD_ID_SIZE_BYTES; i++) {
    hex_stream << std::setw(2) << static_cast<unsigned int>(static_cast<unsigned char>(cs.board_id[i]));
  }

  j = json{{"client_id", cs.client_id},
           {"last_status_time", cs.last_status_time},
           {"board_id", hex_stream.str()}};
}

} // namespace beatled::core

#endif // STATE_MANAGER__CLIENT_STATUS_H