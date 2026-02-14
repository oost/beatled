#include <arpa/inet.h>
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
    SPDLOG_ERROR("Hello request too small: {} bytes", request_buffer_ptr_->size());
    return error_response(BEATLED_ERROR_NO_DATA);
  }

  const auto *hello_req =
      reinterpret_cast<const beatled_message_hello_request_t *>(
          &(request_buffer_ptr_->data()));

  ClientStatus::Ptr cs = std::make_shared<ClientStatus>(
      hello_req->board_id, request_buffer_ptr_->remote_endpoint().address());
  cs->last_status_time = Clock::wall_time_us_64();

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

  auto cs = state_manager_.client_status(
      request_buffer_ptr_->remote_endpoint().address());
  if (cs) {
    cs->last_status_time = Clock::wall_time_us_64();
  }

  const auto *time_req_msg =
      reinterpret_cast<const beatled_message_time_request_t *>(
          &(request_buffer_ptr_->data()));

  uint64_t orig_time = ntohll(time_req_msg->orig_time);

  SPDLOG_INFO("Sending time request. (n) \n - orig_time: {0} / {0:x}",
              orig_time);
  return std::make_unique<TimeResponseBuffer>(orig_time, ms_start,
                                              Clock::time_us_64());
}

DataBuffer::Ptr UDPRequestHandler::process_tempo_request() {
  SPDLOG_INFO("Tempo request");

  auto cs = state_manager_.client_status(
      request_buffer_ptr_->remote_endpoint().address());
  if (cs) {
    cs->last_status_time = Clock::wall_time_us_64();
  }

  tempo_ref_t tr = state_manager_.get_tempo_ref();
  uint16_t pid = state_manager_.get_program_id();

  return std::make_unique<TempoResponseBuffer>(tr.beat_time_ref,
                                               tr.tempo_period_us, pid);
}