#ifndef STATE_MANAGER__CLIENT_STATUS_H
#define STATE_MANAGER__CLIENT_STATUS_H

#include <algorithm> // std::copy
#include <array>
#include <asio.hpp>
#include <cstdint>
#include <fmt/ranges.h>
#include <forward_list>
#include <functional>
#include <iomanip> // std::setw, std::setfill
#include <sstream> // std::ostringstream
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

  // UDP endpoint (address + port) the client should be reached at. Populated
  // from the source endpoint of HELLO_REQUEST and updated on subsequent
  // requests; the broadcaster uses it for unicast delivery.
  asio::ip::udp::endpoint endpoint;

  // Measured one-way delay (server→client) in microseconds, taken from the
  // most recent TIME_REQUEST round-trip as RTT/2. 0 means no sample yet —
  // the broadcaster should fall back to no compensation in that case.
  uint64_t owd_us = 0;

  // Firmware self-description carried on HELLO_REQUEST (protocol v3).
  // `port_name` is one of "pico", "pico-freertos", "posix",
  // "posix-freertos", "esp32", "unknown"; `git_sha` is the short Git SHA
  // (possibly with "-dirty"); `build_time_us` is Unix epoch microseconds
  // of the firmware build. Empty / 0 if the client predates v3 — kept as
  // an explicit fall-through state so the UI can render "—".
  std::string port_name;
  std::string git_sha;
  uint64_t build_time_us = 0;
};

// Manually define to_json for ClientStatus to have full control over board_id serialization
// board_id contains raw binary bytes from the Pico, so we convert to hex string
inline void to_json(json &j, const ClientStatus &cs) {
  // Convert raw binary board_id bytes to hex string representation
  // Always convert exactly 8 bytes (PICO_UNIQUE_BOARD_ID_SIZE_BYTES)
  std::ostringstream hex_stream;
  hex_stream << std::hex << std::setfill('0');

  for (size_t i = 0; i < PICO_UNIQUE_BOARD_ID_SIZE_BYTES; i++) {
    hex_stream << std::setw(2)
               << static_cast<unsigned int>(static_cast<unsigned char>(cs.board_id[i]));
  }

  j = json{{"client_id", cs.client_id},    {"last_status_time", cs.last_status_time},
           {"board_id", hex_stream.str()}, {"port_name", cs.port_name},
           {"git_sha", cs.git_sha},        {"build_time_us", cs.build_time_us}};
}

} // namespace beatled::core

#endif // STATE_MANAGER__CLIENT_STATUS_H