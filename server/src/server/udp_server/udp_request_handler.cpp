#include <arpa/inet.h>
#include <iomanip>
#include <iostream>
#include <string>

#include "beatled/protocol.h"
#include "udp_request_handler.hpp"

using namespace server;

UDPRequestHandler::UDPRequestHandler(UDPRequestBuffer::Ptr &&request_buffer_ptr,
                                     StateManager *state_manager)
    : request_buffer_ptr_{std::move(request_buffer_ptr)}, state_manager_{
                                                              state_manager} {}

UDPResponseBuffer::Ptr UDPRequestHandler::response() {
  // response_buffer_ptr_ = std::make_unique<UDPResponseBuffer>(
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

UDPResponseBuffer::Ptr UDPRequestHandler::error_response(uint8_t error_code) {
  return std::make_unique<UDPErrorResponseBuffer>(error_code);
}

UDPResponseBuffer::Ptr UDPRequestHandler::process_hello_request() {
  std::cout << "Hello request" << std::endl;
  const beatled_message_hello_request_t *hello_req =
      reinterpret_cast<const beatled_message_hello_request_t *>(
          &(request_buffer_ptr_->data()));

  uint16_t pico_id = 1234;
  return std::make_unique<UDPHelloResponseBuffer>(pico_id);
}

UDPResponseBuffer::Ptr UDPRequestHandler::process_time_request() {
  std::cout << "Time request" << std::endl;

  using namespace std::chrono;
  microseconds ms_start =
      duration_cast<microseconds>(system_clock::now().time_since_epoch());

  const beatled_message_time_request_t *time_req_msg =
      reinterpret_cast<const beatled_message_time_request_t *>(
          &(request_buffer_ptr_->data()));

  uint64_t orig_time = ntohll(time_req_msg->orig_time);

  std::cout << "Sending time request. (n) \n - orig_time: " << orig_time
            << std::hex << orig_time << std::endl;
  return std::make_unique<UDPTimeResponseBuffer>(
      orig_time, ms_start.count(),
      duration_cast<microseconds>(system_clock::now().time_since_epoch())
          .count());
}

UDPResponseBuffer::Ptr UDPRequestHandler::process_tempo_request() {
  std::cout << "Tempo request" << std::endl;

  // using namespace std::chrono;

  // float tempo = 100;
  // uint32_t tempo_period_us = 60 * 1000000UL / tempo;
  tempo_ref_t tr = state_manager_->get_tempo_ref();

  return std::make_unique<UDPTempoResponseBuffer>(tr.beat_time_ref,
                                                  tr.tempo_period_us);
}