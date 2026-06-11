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

  // Measured one-way delay (server→client) in microseconds, reported by the
  // controller as median(RTT)/2 on TEMPO_REQUEST. Diagnostic only: beat
  // timestamps travel in the synced-clock domain, so delivery delay must
  // NOT be compensated for (doing so double-counts the path delay).
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

  // Protocol version reported on HELLO_REQUEST. The server only registers
  // clients whose major matches its own, so for any registered client
  // `protocol_version_major == BEATLED_PROTOCOL_VERSION_MAJOR`; the pair is
  // still surfaced for display / debugging.
  uint8_t protocol_version_major = 0;
  uint8_t protocol_version_minor = 0;

  // Latest QoS snapshot received from the controller (protocol v4 — sent
  // piggy-back on every TEMPO_REQUEST and on STATUS_RESPONSE). All
  // controller-supplied values are already decoded into host byte order.
  // `valid` is false until the first qos block arrives; the React UI
  // renders "—" for the new columns in that case.
  struct QosSnapshot {
    bool valid = false;
    int64_t current_offset_us = 0;
    uint64_t uptime_us = 0;
    uint32_t median_rtt_us = 0;
    uint32_t next_beat_gap_total = 0;
    uint32_t intercore_drop_total = 0;
    uint32_t time_sync_outlier_total = 0;
    uint16_t valid_sample_count = 0;
    uint16_t last_applied_program_seq = 0;
    // Server-side bookkeeping not carried on the wire.
    uint64_t server_received_at_us = 0;
    uint32_t last_rtt_us = 0; // populated from STATUS_RESPONSE
  };
  QosSnapshot latest_qos;
};

// Server-side estimate of a snapshot's clock-sync error, in microseconds.
// The controller samples uptime_us when it builds the QoS block; the server
// stamps server_received_at_us on arrival, one OWD (~RTT/2) later. So the
// server's independent view of the controller's offset is
//   (server_received_at_us - rtt/2) - uptime_us
// and the difference from the controller's own current_offset_us is its
// sync error. The spread of these errors across the fleet approximates the
// real beat skew (each device's raw offset is dominated by its boot epoch
// and is useless for cross-device comparison). Returns false when the
// snapshot lacks the inputs (no probe seen yet / no RTT sample).
inline bool qos_sync_error_us(const ClientStatus::QosSnapshot &q, int64_t &error_us) {
  const uint32_t rtt = q.last_rtt_us > 0 ? q.last_rtt_us : q.median_rtt_us;
  if (!q.valid || q.server_received_at_us == 0 || rtt == 0) {
    return false;
  }
  const int64_t server_view =
      static_cast<int64_t>(q.server_received_at_us - rtt / 2) - static_cast<int64_t>(q.uptime_us);
  error_us = q.current_offset_us - server_view;
  return true;
}

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

  j = json{{"client_id", cs.client_id},
           {"last_status_time", cs.last_status_time},
           {"board_id", hex_stream.str()},
           {"port_name", cs.port_name},
           {"git_sha", cs.git_sha},
           {"build_time_us", cs.build_time_us},
           {"protocol_version_major", cs.protocol_version_major},
           {"protocol_version_minor", cs.protocol_version_minor},
           {"owd_us", cs.owd_us}};
  if (cs.latest_qos.valid) {
    j["qos"] = json{
        {"current_offset_us", cs.latest_qos.current_offset_us},
        {"uptime_us", cs.latest_qos.uptime_us},
        {"median_rtt_us", cs.latest_qos.median_rtt_us},
        {"next_beat_gap_total", cs.latest_qos.next_beat_gap_total},
        {"intercore_drop_total", cs.latest_qos.intercore_drop_total},
        {"time_sync_outlier_total", cs.latest_qos.time_sync_outlier_total},
        {"valid_sample_count", cs.latest_qos.valid_sample_count},
        {"last_applied_program_seq", cs.latest_qos.last_applied_program_seq},
        {"server_received_at_us", cs.latest_qos.server_received_at_us},
        {"last_rtt_us", cs.latest_qos.last_rtt_us},
    };
    int64_t sync_error_us = 0;
    if (qos_sync_error_us(cs.latest_qos, sync_error_us)) {
      j["qos"]["sync_error_us"] = sync_error_us;
    } else {
      j["qos"]["sync_error_us"] = nullptr;
    }
  } else {
    j["qos"] = nullptr;
  }
}

} // namespace beatled::core

#endif // STATE_MANAGER__CLIENT_STATUS_H