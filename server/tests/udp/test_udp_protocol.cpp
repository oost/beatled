#include <arpa/inet.h>
#include <catch2/catch_test_macros.hpp>
#include <cstring>

#include "beatled/network.h"
#include "beatled/protocol.h"
#include "udp/udp_buffer.hpp"

using namespace beatled::server;

// --- ErrorResponseBuffer ---

TEST_CASE("ErrorResponseBuffer serialization", "[udp][protocol]") {
  SECTION("Type byte and error code are correct") {
    ErrorResponseBuffer buf(BEATLED_ERROR_NO_DATA);

    REQUIRE(buf.size() == sizeof(beatled_message_error_t));
    REQUIRE(buf.type() == BEATLED_MESSAGE_ERROR);

    const auto *msg =
        reinterpret_cast<const beatled_message_error_t *>(&buf.data());
    REQUIRE(msg->error_code == BEATLED_ERROR_NO_DATA);
  }

  SECTION("Different error codes are preserved") {
    ErrorResponseBuffer buf(BEATLED_ERROR_UNKNOWN_MESSAGE_TYPE);
    const auto *msg =
        reinterpret_cast<const beatled_message_error_t *>(&buf.data());
    REQUIRE(msg->error_code == BEATLED_ERROR_UNKNOWN_MESSAGE_TYPE);
  }
}

// --- HelloResponseBuffer ---

TEST_CASE("HelloResponseBuffer serialization", "[udp][protocol]") {
  uint16_t client_id = 0x1234;
  HelloResponseBuffer buf(client_id);

  REQUIRE(buf.size() == sizeof(beatled_message_hello_response_t));
  REQUIRE(buf.type() == BEATLED_MESSAGE_HELLO_RESPONSE);

  const auto *msg =
      reinterpret_cast<const beatled_message_hello_response_t *>(&buf.data());
  REQUIRE(ntohs(msg->client_id) == client_id);
}

// --- TimeResponseBuffer ---

TEST_CASE("TimeResponseBuffer serialization", "[udp][protocol]") {
  uint64_t orig = 1000000ULL;
  uint64_t recv = 2000000ULL;
  uint64_t xmit = 3000000ULL;

  TimeResponseBuffer buf(orig, recv, xmit);

  REQUIRE(buf.size() == sizeof(beatled_message_time_response_t));
  REQUIRE(buf.type() == BEATLED_MESSAGE_TIME_RESPONSE);

  const auto *msg =
      reinterpret_cast<const beatled_message_time_response_t *>(&buf.data());
  REQUIRE(ntohll(msg->orig_time) == orig);
  REQUIRE(ntohll(msg->recv_time) == recv);
  REQUIRE(ntohll(msg->xmit_time) == xmit);
}

// --- TempoResponseBuffer ---

TEST_CASE("TempoResponseBuffer serialization", "[udp][protocol]") {
  uint64_t beat_time_ref = 0xDEADBEEF12345678ULL;
  uint32_t tempo_period_us = 500000;
  uint16_t program_id = 42;

  TempoResponseBuffer buf(beat_time_ref, tempo_period_us, program_id);

  REQUIRE(buf.size() == sizeof(beatled_message_tempo_response_t));
  REQUIRE(buf.type() == BEATLED_MESSAGE_TEMPO_RESPONSE);

  const auto *msg =
      reinterpret_cast<const beatled_message_tempo_response_t *>(&buf.data());
  REQUIRE(ntohll(msg->beat_time_ref) == beat_time_ref);
  REQUIRE(ntohl(msg->tempo_period_us) == tempo_period_us);
  REQUIRE(ntohs(msg->program_id) == program_id);
}

// --- NextBeatBuffer ---

TEST_CASE("NextBeatBuffer serialization", "[udp][protocol]") {
  uint64_t next_beat_time_ref = 9999999ULL;
  uint32_t tempo_period_us = 500000;
  uint32_t beat_count = 100;
  uint16_t program_id = 3;

  NextBeatBuffer buf(next_beat_time_ref, tempo_period_us, beat_count,
                     program_id);

  REQUIRE(buf.size() == sizeof(beatled_message_next_beat_t));
  REQUIRE(buf.type() == BEATLED_MESSAGE_NEXT_BEAT);

  const auto *msg =
      reinterpret_cast<const beatled_message_next_beat_t *>(&buf.data());
  REQUIRE(ntohll(msg->next_beat_time_ref) == next_beat_time_ref);
  REQUIRE(ntohl(msg->tempo_period_us) == tempo_period_us);
  REQUIRE(ntohl(msg->beat_count) == beat_count);
  REQUIRE(ntohs(msg->program_id) == program_id);
}

// --- BeatBuffer ---

TEST_CASE("BeatBuffer serialization", "[udp][protocol]") {
  uint64_t beat_time_ref = 8888888ULL;
  uint32_t tempo_period_us = 333333;
  uint32_t beat_count = 50;
  uint16_t program_id = 7;

  BeatBuffer buf(beat_time_ref, tempo_period_us, beat_count, program_id);

  REQUIRE(buf.size() == sizeof(beatled_message_beat_t));
  REQUIRE(buf.type() == BEATLED_MESSAGE_BEAT);

  const auto *msg =
      reinterpret_cast<const beatled_message_beat_t *>(&buf.data());
  REQUIRE(ntohll(msg->beat_time_ref) == beat_time_ref);
  REQUIRE(ntohl(msg->tempo_period_us) == tempo_period_us);
  REQUIRE(ntohl(msg->beat_count) == beat_count);
  REQUIRE(ntohs(msg->program_id) == program_id);
}

// --- UDPRequestBuffer ---

TEST_CASE("UDPRequestBuffer size validation", "[udp][protocol]") {
  UDPRequestBuffer buf;

  SECTION("Valid size is accepted") {
    buf.setSize(100);
    REQUIRE(buf.size() == 100);
  }

  SECTION("Max size is accepted") {
    buf.setSize(DataBuffer::BUFFER_SIZE);
    REQUIRE(buf.size() == DataBuffer::BUFFER_SIZE);
  }

  SECTION("Overflow throws") {
    REQUIRE_THROWS_AS(buf.setSize(DataBuffer::BUFFER_SIZE + 1),
                      std::overflow_error);
  }
}

// --- DataBuffer::type() ---

TEST_CASE("DataBuffer type returns first byte", "[udp][protocol]") {
  SECTION("Type returns first byte of populated buffer") {
    HelloResponseBuffer buf(1);
    REQUIRE(buf.type() == BEATLED_MESSAGE_HELLO_RESPONSE);
  }

  SECTION("Empty buffer throws on type()") {
    UDPRequestBuffer buf;
    REQUIRE_THROWS_AS(buf.type(), std::range_error);
  }
}
