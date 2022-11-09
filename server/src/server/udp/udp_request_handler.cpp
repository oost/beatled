#include <arpa/inet.h>
#include <iomanip>
#include <iostream>
#include <string>

#include "beatled/protocol.h"
#include "udp_request_handler.hpp"

using namespace server;

UDPRequestHandler::UDPRequestHandler(UDPRequestBuffer::Ptr request_buffer_ptr)
    : request_buffer_ptr_{std::move(request_buffer_ptr)} {}

UDPResponseBuffer::Ptr UDPRequestHandler::response() {

  if (request_buffer_ptr_->size() == 0) {

    return error_response(noData);

  } else {
    switch (request_buffer_ptr_->type()) {
    case eBeatledHello:
      return process_hello_request();
    case eBeatledTime:
      return process_time_request();

    case eBeatledTempo:
      return process_tempo_request();

    default:
      return error_response(unknownMessageType);
    }
  }
}

template <typename T>
UDPResponseBuffer::Ptr UDPRequestHandler::make_response_buffer(const T &data) {
  UDPResponseBuffer::Ptr buffer_ptr = std::make_unique<UDPResponseBuffer>();
  memcpy(&(buffer_ptr->data()), &data, sizeof(T));
  buffer_ptr->setSize(sizeof(T));
  return buffer_ptr;
}

UDPResponseBuffer::Ptr UDPRequestHandler::error_response(uint8_t error_code) {
  beatled_error_msg_t error_message;
  error_message.base.type = eBeatledError;
  error_message.error_code = error_code;

  return make_response_buffer(error_message);
}

UDPResponseBuffer::Ptr UDPRequestHandler::process_hello_request() {
  std::cout << "Hello request" << std::endl;
  const beatled_hello_msg_t *hello_req =
      reinterpret_cast<const beatled_hello_msg_t *>(
          &(request_buffer_ptr_->data()));

  beatled_hello_response_t response;
  response.base.type = eBeatledHello;
  // response.pico_id = hello_req->pico_id; // Should be the actual id of the
  // pico

  return make_response_buffer(response);
}

UDPResponseBuffer::Ptr UDPRequestHandler::process_time_request() {
  std::cout << "Time request" << std::endl;

  using namespace std::chrono;
  microseconds ms_start =
      duration_cast<microseconds>(system_clock::now().time_since_epoch());

  const beatled_time_req_msg_t *time_req_msg =
      reinterpret_cast<const beatled_time_req_msg_t *>(
          &(request_buffer_ptr_->data()));

  beatled_time_resp_msg_t time_resp_msg;
  time_resp_msg.base.type = eBeatledTime;
  time_resp_msg.orig_time = time_req_msg->orig_time;
  time_resp_msg.recv_time = htonll(ms_start.count());
  time_resp_msg.xmit_time =
      htonll(duration_cast<microseconds>(system_clock::now().time_since_epoch())
                 .count());
  uint64_t orig_time = time_req_msg->orig_time;

  std::cout << "Sending time request. (n) \n - orig_time: " << orig_time
            << std::hex << orig_time << std::endl;
  std::cout << "Sending time request (h). \n - orig_time: " << ntohll(orig_time)
            << std::hex << ntohll(orig_time) << std::endl;
  return make_response_buffer(time_resp_msg);
}

UDPResponseBuffer::Ptr UDPRequestHandler::process_tempo_request() {
  std::cout << "Tempo request" << std::endl;

  using namespace std::chrono;

  float tempo = 100;
  uint32_t tempo_period_us = 60 * 1000000UL / tempo;

  beatled_tempo_msg_t tempo_msg;
  tempo_msg.base.type = eBeatledTempo;
  tempo_msg.beat_time_ref =
      htonll(duration_cast<microseconds>(system_clock::now().time_since_epoch())
                 .count());
  tempo_msg.tempo_period_us = htonl(tempo_period_us);

  return make_response_buffer(tempo_msg);
}