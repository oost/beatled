#include <iostream>

#include "beatled/protocol.h"
#include "udp_buffer.hpp"

namespace server {

template <typename T> void UDPResponseBuffer::set_data(const T &data) {
  memcpy(&this->data(), &data, sizeof(T));
  setSize(sizeof(T));
}

void UDPResponseBuffer::set_error_response(uint8_t error_code) {
  beatled_message_error_t error_message;
  error_message.base.type = BEATLED_MESSAGE_ERROR;
  error_message.error_code = error_code;

  set_data(error_message);
}

void UDPResponseBuffer::set_hello_response(uint16_t pico_id) {
  beatled_message_hello_response_t response;
  response.base.type = BEATLED_MESSAGE_HELLO_RESPONSE;
  response.pico_id = htons(pico_id);

  set_data(response);
}

void UDPResponseBuffer::set_time_response(uint64_t orig_time,
                                          uint64_t recv_time,
                                          uint64_t xmit_time) {

  beatled_message_time_response_t time_resp_msg;
  time_resp_msg.base.type = BEATLED_MESSAGE_TIME_RESPONSE;
  time_resp_msg.orig_time = htonll(orig_time);
  time_resp_msg.recv_time = htonll(recv_time);
  time_resp_msg.xmit_time = htonll(xmit_time);

  return set_data(time_resp_msg);
}

void UDPResponseBuffer::set_tempo_response(uint64_t beat_time_ref,
                                           uint32_t tempo_period_us) {
  beatled_message_tempo_response_t tempo_msg;
  tempo_msg.base.type = BEATLED_MESSAGE_TEMPO_RESPONSE;
  tempo_msg.beat_time_ref = htonll(beat_time_ref);
  tempo_msg.tempo_period_us = htonl(tempo_period_us);

  return set_data(tempo_msg);
}
} // namespace server