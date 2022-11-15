#include <arpa/inet.h>
#include <iomanip>
#include <iostream>
#include <string>

#include "beatled/protocol.h"
#include "udp_request_handler.hpp"

using namespace server;

UDPRequestHandler::UDPRequestHandler(UDPRequestBuffer::Ptr &&request_buffer_ptr)
    : request_buffer_ptr_{std::move(request_buffer_ptr)} {}

UDPResponseBuffer::Ptr UDPRequestHandler::response() {
  response_buffer_ptr_ = std::make_unique<UDPResponseBuffer>(
      request_buffer_ptr_->remote_endpoint());

  if (request_buffer_ptr_->size() == 0) {

    error_response(BEATLED_ERROR_NO_DATA);

  } else {
    switch (request_buffer_ptr_->type()) {
    case BEATLED_MESSAGE_HELLO_REQUEST:
      process_hello_request();
      break;

    case BEATLED_MESSAGE_TIME_REQUEST:
      process_time_request();
      break;

    case BEATLED_MESSAGE_TEMPO_REQUEST:
      process_tempo_request();
      break;

    default:
      error_response(BEATLED_ERROR_UNKNOWN_MESSAGE_TYPE);
      break;
    }
  }
  return std::move(response_buffer_ptr_);
}

void UDPRequestHandler::error_response(uint8_t error_code) {
  response_buffer_ptr_->set_error_response(error_code);
}

void UDPRequestHandler::process_hello_request() {
  std::cout << "Hello request" << std::endl;
  const beatled_message_hello_request_t *hello_req =
      reinterpret_cast<const beatled_message_hello_request_t *>(
          &(request_buffer_ptr_->data()));

  uint16_t pico_id = 1234;
  response_buffer_ptr_->set_hello_response(pico_id);
}

void UDPRequestHandler::process_time_request() {
  std::cout << "Time request" << std::endl;

  using namespace std::chrono;
  microseconds ms_start =
      duration_cast<microseconds>(system_clock::now().time_since_epoch());

  const beatled_message_time_request_t *time_req_msg =
      reinterpret_cast<const beatled_message_time_request_t *>(
          &(request_buffer_ptr_->data()));

  uint64_t orig_time = ntohll(time_req_msg->orig_time);
  response_buffer_ptr_->set_time_response(
      orig_time, ms_start.count(),
      duration_cast<microseconds>(system_clock::now().time_since_epoch())
          .count());

  std::cout << "Sending time request. (n) \n - orig_time: " << orig_time
            << std::hex << orig_time << std::endl;
}

void UDPRequestHandler::process_tempo_request() {
  std::cout << "Tempo request" << std::endl;

  using namespace std::chrono;

  float tempo = 100;
  uint32_t tempo_period_us = 60 * 1000000UL / tempo;

  response_buffer_ptr_->set_tempo_response(
      duration_cast<microseconds>(system_clock::now().time_since_epoch())
          .count(),
      tempo_period_us);
}