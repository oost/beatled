#include <arpa/inet.h>
#include <iomanip>
#include <iostream>
#include <spdlog/spdlog.h>
#include <string>

#include "beatled/protocol.h"
#include "udp_request_handler.hpp"

using namespace server;

UDPRequestHandler::UDPRequestHandler(UDPRequestBuffer *request_buffer_ptr,
                                     StateManager &state_manager)
    : request_buffer_ptr_{request_buffer_ptr}, state_manager_{state_manager} {}

ResponseBuffer::Ptr UDPRequestHandler::response() {
  // response_buffer_ptr_ = std::make_unique<ResponseBuffer>(
  // request_buffer_ptr_->remote_endpoint());

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

ResponseBuffer::Ptr UDPRequestHandler::error_response(uint8_t error_code) {
  return std::make_unique<ErrorResponseBuffer>(error_code);
}

ResponseBuffer::Ptr UDPRequestHandler::process_hello_request() {
  SPDLOG_INFO("Hello request");
  const beatled_message_hello_request_t *hello_req =
      reinterpret_cast<const beatled_message_hello_request_t *>(
          &(request_buffer_ptr_->data()));

  ClientStatus::board_id_t board_id;
  std::copy(hello_req->board_id, hello_req->board_id + board_id.size(),
            board_id.begin());

  ClientStatus *cs = state_manager_.register_client(
      request_buffer_ptr_->remote_endpoint().address(), board_id);
  if (!cs) {
    // FIXME: Handle error
    SPDLOG_ERROR("Address already registered");
    return std::make_unique<HelloResponseBuffer>(0);
  } else {
    return std::make_unique<HelloResponseBuffer>(cs->client_id);
  }
}

ResponseBuffer::Ptr UDPRequestHandler::process_time_request() {
  SPDLOG_INFO("Time request");

  using namespace std::chrono;
  microseconds ms_start =
      duration_cast<microseconds>(system_clock::now().time_since_epoch());

  const beatled_message_time_request_t *time_req_msg =
      reinterpret_cast<const beatled_message_time_request_t *>(
          &(request_buffer_ptr_->data()));

  uint64_t orig_time = ntohll(time_req_msg->orig_time);

  SPDLOG_INFO("Sending time request. (n) \n - orig_time: {0} / {0:x}",
              orig_time);
  return std::make_unique<TimeResponseBuffer>(
      orig_time, ms_start.count(),
      duration_cast<microseconds>(system_clock::now().time_since_epoch())
          .count());
}

ResponseBuffer::Ptr UDPRequestHandler::process_tempo_request() {
  SPDLOG_INFO("Tempo request");

  // using namespace std::chrono;

  // float tempo = 100;
  // uint32_t tempo_period_us = 60 * 1000000UL / tempo;
  tempo_ref_t tr = state_manager_.get_tempo_ref();

  return std::make_unique<TempoResponseBuffer>(tr.beat_time_ref,
                                               tr.tempo_period_us);
}