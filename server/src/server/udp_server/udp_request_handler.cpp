#include <arpa/inet.h>
#include <cstring>
#include <spdlog/spdlog.h>
#include <string>

#include "beatled/network.h"
#include "beatled/protocol.h"
#include "core/clock.hpp"
#include "udp_request_handler.hpp"

using beatled::core::ClientStatus;
using beatled::core::Clock;
using beatled::core::tempo_ref_t;

using namespace beatled::server;

UDPRequestHandler::UDPRequestHandler(UDPRequestBuffer *request_buffer_ptr,
                                     StateManager &state_manager)
    : request_buffer_ptr_{request_buffer_ptr}, state_manager_{state_manager} {}

namespace {

// Decode the wire-format `beatled_qos_block_t` (network byte order) into
// the host-byte-order ClientStatus::QosSnapshot. Marks the snapshot
// valid and stamps server_received_at_us with the current wall time so
// the React UI can show "QoS sample N ms old".
void decode_qos_block(const beatled_qos_block_t &qos_in, ClientStatus::QosSnapshot &qos) {
  qos.valid = true;
  qos.server_received_at_us = Clock::wall_time_us_64();
  uint64_t offset_bits = ntohll(static_cast<uint64_t>(qos_in.current_offset_us));
  std::memcpy(&qos.current_offset_us, &offset_bits, sizeof(qos.current_offset_us));
  qos.uptime_us = ntohll(qos_in.uptime_us);
  qos.median_rtt_us = ntohl(qos_in.median_rtt_us);
  qos.next_beat_gap_total = ntohl(qos_in.next_beat_gap_total);
  qos.intercore_drop_total = ntohl(qos_in.intercore_drop_total);
  qos.time_sync_outlier_total = ntohl(qos_in.time_sync_outlier_total);
  qos.valid_sample_count = ntohs(qos_in.valid_sample_count);
  qos.last_applied_program_seq = ntohs(qos_in.last_applied_program_seq);
}

} // namespace

DataBuffer::Ptr UDPRequestHandler::response() {

  if (request_buffer_ptr_->size() == 0) {

    return error_response(BEATLED_ERROR_NO_DATA);

  } else {
    switch (request_buffer_ptr_->type()) {
    case BEATLED_MESSAGE_HELLO_REQUEST:
      return process_hello_request();

    case BEATLED_MESSAGE_TIME_REQUEST:
      return process_time_request();

    case BEATLED_MESSAGE_TEMPO_REQUEST:
      return process_tempo_request();

    case BEATLED_MESSAGE_STATUS_RESPONSE:
      return process_status_response();

    default:
      return error_response(BEATLED_ERROR_UNKNOWN_MESSAGE_TYPE);
    }
  }
}

DataBuffer::Ptr UDPRequestHandler::error_response(uint8_t error_code) {
  return std::make_unique<ErrorResponseBuffer>(error_code);
}

DataBuffer::Ptr UDPRequestHandler::process_hello_request() {
  SPDLOG_INFO("Hello request");

  if (request_buffer_ptr_->size() < sizeof(beatled_message_hello_request_t)) {
    SPDLOG_ERROR("Hello request too small: {} bytes (expected {} for v3)",
                 request_buffer_ptr_->size(), sizeof(beatled_message_hello_request_t));
    return error_response(BEATLED_ERROR_NO_DATA);
  }

  const auto *hello_req =
      reinterpret_cast<const beatled_message_hello_request_t *>(&(request_buffer_ptr_->data()));

  ClientStatus::Ptr cs = std::make_shared<ClientStatus>(
      hello_req->board_id, request_buffer_ptr_->remote_endpoint().address());
  cs->last_status_time = Clock::wall_time_us_64();
  // Remember the full endpoint (incl. ephemeral port) for unicast delivery.
  cs->endpoint = request_buffer_ptr_->remote_endpoint();

  // Pull the firmware self-description carried since protocol v3. The
  // wire fields are fixed-size, null-terminated strings; build_time_us
  // is network byte order.
  cs->port_name =
      std::string(hello_req->port_name, strnlen(hello_req->port_name, BEATLED_PORT_NAME_LEN));
  cs->git_sha = std::string(hello_req->git_sha, strnlen(hello_req->git_sha, BEATLED_GIT_HASH_LEN));
  cs->build_time_us = ntohll(hello_req->build_time_us);
  SPDLOG_INFO("Client hello: board_id={} port={} sha={} built_us={}", hello_req->board_id,
              cs->port_name, cs->git_sha, cs->build_time_us);

  state_manager_.register_client(cs);
  return std::make_unique<HelloResponseBuffer>(cs->client_id);
}

DataBuffer::Ptr UDPRequestHandler::process_time_request() {
  SPDLOG_INFO("Time request");

  if (request_buffer_ptr_->size() < sizeof(beatled_message_time_request_t)) {
    SPDLOG_ERROR("Time request too small: {} bytes", request_buffer_ptr_->size());
    return error_response(BEATLED_ERROR_NO_DATA);
  }

  using namespace std::chrono;
  uint64_t ms_start = Clock::time_us_64();

  auto cs = state_manager_.client_status(request_buffer_ptr_->remote_endpoint().address());
  if (cs) {
    cs->last_status_time = Clock::wall_time_us_64();
  }

  const auto *time_req_msg =
      reinterpret_cast<const beatled_message_time_request_t *>(&(request_buffer_ptr_->data()));

  uint64_t orig_time = ntohll(time_req_msg->orig_time);

  SPDLOG_INFO("Sending time request. (n) \n - orig_time: {0} / {0:x}", orig_time);
  return std::make_unique<TimeResponseBuffer>(orig_time, ms_start, Clock::time_us_64());
}

DataBuffer::Ptr UDPRequestHandler::process_tempo_request() {
  SPDLOG_INFO("Tempo request");

  if (request_buffer_ptr_->size() < sizeof(beatled_message_tempo_request_t)) {
    SPDLOG_ERROR("Tempo request too small: {} bytes", request_buffer_ptr_->size());
    return error_response(BEATLED_ERROR_NO_DATA);
  }

  const auto *tempo_req =
      reinterpret_cast<const beatled_message_tempo_request_t *>(&(request_buffer_ptr_->data()));
  const auto remote = request_buffer_ptr_->remote_endpoint();
  uint32_t owd_us = ntohl(tempo_req->owd_us_estimate);

  auto cs = state_manager_.client_status(remote.address());
  if (cs) {
    cs->last_status_time = Clock::wall_time_us_64();
    // Track the latest endpoint too (the ephemeral source port can shift on
    // FreeRTOS sockets across reboots).
    cs->endpoint = remote;
    // Protocol v4: decode the trailing diagnostic block into
    // ClientStatus::QosSnapshot. STATUS_RESPONSE shares the same helper.
    decode_qos_block(tempo_req->qos, cs->latest_qos);
  }
  if (owd_us > 0) {
    state_manager_.update_client_owd(remote.address(), owd_us);
  }

  tempo_ref_t tr = state_manager_.get_tempo_ref();
  uint16_t pid = state_manager_.get_program_id();

  return std::make_unique<TempoResponseBuffer>(tr.beat_time_ref, tr.tempo_period_us, pid);
}

DataBuffer::Ptr UDPRequestHandler::process_status_response() {
  if (request_buffer_ptr_->size() < sizeof(beatled_message_status_response_t)) {
    SPDLOG_ERROR("Status response too small: {} bytes (expected {})", request_buffer_ptr_->size(),
                 sizeof(beatled_message_status_response_t));
    return error_response(BEATLED_ERROR_NO_DATA);
  }
  const auto *resp =
      reinterpret_cast<const beatled_message_status_response_t *>(&(request_buffer_ptr_->data()));
  const auto remote = request_buffer_ptr_->remote_endpoint();

  auto cs = state_manager_.client_status(remote.address());
  if (cs) {
    cs->last_status_time = Clock::wall_time_us_64();
    cs->endpoint = remote;
    decode_qos_block(resp->qos, cs->latest_qos);
    // Fresh server-controlled RTT measurement: now − echoed send-time.
    const uint64_t send_time = ntohll(resp->echo_server_send_time_us);
    const uint64_t now = Clock::wall_time_us_64();
    if (now > send_time) {
      const uint64_t rtt = now - send_time;
      cs->latest_qos.last_rtt_us = rtt > UINT32_MAX ? UINT32_MAX : static_cast<uint32_t>(rtt);
    }
    SPDLOG_DEBUG("Status response from {}: rtt_us={} median_rtt_us={}",
                 remote.address().to_string(), cs->latest_qos.last_rtt_us,
                 cs->latest_qos.median_rtt_us);
  } else {
    // STATUS_RESPONSE from an unknown source — controller hasn't said
    // HELLO yet, or we restarted between probe and reply. Log + drop.
    SPDLOG_INFO("Status response from unknown client {}", remote.address().to_string());
  }

  // STATUS_RESPONSE is the terminal half of the probe — no further
  // server-to-client reply needed. Returning nullptr tells udp_server to
  // skip the async_send_to.
  return nullptr;
}