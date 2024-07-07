#include <arpa/inet.h>
#include <iostream>

#include "beatled/network.h"
#include "beatled/protocol.h"
#include "udp/udp_buffer.hpp"

namespace beatled::server {

ErrorResponseBuffer::ErrorResponseBuffer(uint8_t error_code)
    : ResponseBuffer() {
  beatled_message_error_t error_message;
  error_message.base.type = BEATLED_MESSAGE_ERROR;
  error_message.error_code = error_code;

  set_data(error_message);
}

HelloResponseBuffer::HelloResponseBuffer(uint16_t client_id)
    : ResponseBuffer() {
  beatled_message_hello_response_t response;
  response.base.type = BEATLED_MESSAGE_HELLO_RESPONSE;
  response.client_id = htons(client_id);

  set_data(response);
}

TimeResponseBuffer::TimeResponseBuffer(uint64_t orig_time, uint64_t recv_time,
                                       uint64_t xmit_time) {
  beatled_message_time_response_t time_resp_msg;
  time_resp_msg.base.type = BEATLED_MESSAGE_TIME_RESPONSE;
  time_resp_msg.orig_time = htonll(orig_time);
  time_resp_msg.recv_time = htonll(recv_time);
  time_resp_msg.xmit_time = htonll(xmit_time);

  set_data(time_resp_msg);
}

TempoResponseBuffer::TempoResponseBuffer(uint64_t beat_time_ref,
                                         uint32_t tempo_period_us,
                                         uint16_t program_id)
    : ResponseBuffer() {
  beatled_message_tempo_response_t tempo_msg;
  tempo_msg.base.type = BEATLED_MESSAGE_TEMPO_RESPONSE;
  tempo_msg.beat_time_ref = htonll(beat_time_ref);
  tempo_msg.tempo_period_us = htonl(tempo_period_us);
  tempo_msg.program_id = htons(program_id);

  set_data(tempo_msg);
}

NextBeatBuffer::NextBeatBuffer(uint64_t next_beat_time_ref,
                               uint32_t tempo_period_us, uint32_t beat_count,
                               uint16_t program_id) {
  beatled_message_next_beat_t bext_beat_msg;
  bext_beat_msg.base.type = BEATLED_MESSAGE_NEXT_BEAT;
  bext_beat_msg.next_beat_time_ref = htonll(next_beat_time_ref);
  bext_beat_msg.tempo_period_us = htonl(tempo_period_us);
  bext_beat_msg.beat_count = htonl(beat_count);
  bext_beat_msg.program_id = htons(program_id);

  set_data(bext_beat_msg);
};

BeatBuffer::BeatBuffer(uint64_t beat_time_ref, uint32_t tempo_period_us,
                       uint32_t beat_count, uint16_t program_id) {
  beatled_message_beat_t bext_beat_msg;
  bext_beat_msg.base.type = BEATLED_MESSAGE_BEAT;
  bext_beat_msg.beat_time_ref = htonll(beat_time_ref);
  bext_beat_msg.tempo_period_us = htonl(tempo_period_us);
  bext_beat_msg.beat_count = htonl(beat_count);
  bext_beat_msg.program_id = htons(program_id);

  set_data(bext_beat_msg);
};

} // namespace beatled::server