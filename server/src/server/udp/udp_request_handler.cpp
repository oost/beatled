#include <arpa/inet.h>
#include <iostream>
#include <string>

#include "beatled/protocol.h"
#include "udp_request_handler.hpp"

using namespace server;

UDPRequestHandler::UDPRequestHandler(
    std::string_view request_body,
    const asio::ip::udp::endpoint &remote_endpoint)
    : request_body_{request_body}, remote_endpoint_{remote_endpoint} {}

std::string UDPRequestHandler::get_response() {
  std::string response_body;

  std::cout << "Received: " << request_body_ << std::endl;

  std::string response = "E0";
  if (request_body_.size() == 0) {
    response_body = "E1";
  } else {
    switch (request_body_[0]) {
    case 'T':
      response_body = time_response();
      break;
    case 't':
      response_body = tempo();
      break;
    default:
      response_body = "E2";
      break;
    }
  }

  return response_body;

  // TODO : Is this correct? Does the string need to be moved?
  // std::cout << "Sending: " << response_body_ << std::endl;

  // socket_.async_send_to(
  //     asio::buffer(response_body_), remote_endpoint,
  //     [this, self](std::error_code /*ec*/, std::size_t /*bytes_sent*/) {});
}

std::string serialize_uint32_t(uint32_t val) {
  std::string ret = "0000";
  ret[0] = val;
  ret[1] = val >> 8;
  ret[2] = val >> 16;
  ret[3] = val >> 24;
  return ret;
}

std::string serialize_uint64_t(uint64_t val) {
  std::string ret = "00000000";
  ret[0] = val;
  ret[1] = val >> 8;
  ret[2] = val >> 16;
  ret[3] = val >> 24;
  ret[4] = val >> 32;
  ret[5] = val >> 40;
  ret[6] = val >> 48;
  ret[7] = val >> 56;
  return ret;
}

std::string UDPRequestHandler::time_response() {
  time_resp_msg_t time_resp_msg;
  time_resp_msg.command = COMMAND_TIME;
  time_resp_msg.orig_time = htonll(0);
  time_resp_msg.rec_time = htonll(0);
  time_resp_msg.xmit_time = htonll(0);

  return std::string(reinterpret_cast<char *>(&time_resp_msg));
}

std::string UDPRequestHandler::tempo() {
  float tempo = 100;
  uint32_t tempo_period_us = 60 * 1000000UL / tempo;

  tempo_msg_t tempo_msg;
  tempo_msg.command = COMMAND_TEMPO;
  tempo_msg.beat_time_ref = htonll(0);
  tempo_msg.tempo_period_us = htonl(tempo_period_us);

  return std::string(reinterpret_cast<char *>(&tempo_msg));
}