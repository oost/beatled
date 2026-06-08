#include <catch2/catch_test_macros.hpp>
#include <cstddef> // offsetof

#ifdef __cplusplus
extern "C" {
#endif

#include "beatled/protocol.h"

#ifdef __cplusplus
}
#endif

TEST_CASE("Protocol struct sizes match wire format", "[protocol]") {

  SECTION("Base message is 1 byte") {
    REQUIRE(sizeof(beatled_message_t) == 1);
  }

  SECTION("Error message is 2 bytes") {
    REQUIRE(sizeof(beatled_message_error_t) == 2);
  }

  SECTION("Hello request: version handshake + board_id + firmware self-description") {
    // 1 type + 2 version (major,minor) + 17 board_id (2*8+1) + 16 port_name
    // + 16 git_sha + 8 build_time_us = 60 bytes total.
    REQUIRE(sizeof(beatled_message_hello_request_t) == 60);
  }

  SECTION("Protocol version constants are defined") {
    REQUIRE(BEATLED_PROTOCOL_VERSION_MAJOR >= 1);
    // version_major / version_minor lead the payload at fixed offsets so
    // any server can read a peer's major version.
    REQUIRE(offsetof(beatled_message_hello_request_t, version_major) == 1);
    REQUIRE(offsetof(beatled_message_hello_request_t, version_minor) == 2);
  }

  SECTION("Hello response is 3 bytes") {
    // 1 byte type + 2 bytes client_id
    REQUIRE(sizeof(beatled_message_hello_response_t) == 3);
  }

  SECTION("Time request is 9 bytes") {
    // 1 byte type + 8 bytes orig_time
    REQUIRE(sizeof(beatled_message_time_request_t) == 9);
  }

  SECTION("Time response is 25 bytes") {
    // 1 byte type + 8 orig + 8 recv + 8 xmit
    REQUIRE(sizeof(beatled_message_time_response_t) == 25);
  }

  SECTION("QoS block is 36 bytes (v4)") {
    // int64 offset (8) + uint64 uptime (8) + 4 * uint32 (16) + 2 * uint16 (4)
    REQUIRE(sizeof(beatled_qos_block_t) == 36);
  }

  SECTION("Tempo request is 41 bytes (v4)") {
    // 1 byte type + 4 bytes owd_us_estimate + 36 byte qos block
    REQUIRE(sizeof(beatled_message_tempo_request_t) == 41);
  }

  SECTION("Status request is 9 bytes (v4)") {
    // 1 byte type + 8 bytes server_send_time_us
    REQUIRE(sizeof(beatled_message_status_request_t) == 9);
  }

  SECTION("Status response is 45 bytes (v4)") {
    // 1 byte type + 8 echo + 36 qos block
    REQUIRE(sizeof(beatled_message_status_response_t) == 45);
  }

  SECTION("Tempo response is 15 bytes") {
    // 1 byte type + 8 beat_time_ref + 4 tempo_period_us + 2 program_id
    REQUIRE(sizeof(beatled_message_tempo_response_t) == 15);
  }

  SECTION("Program message is 5 bytes (v2)") {
    // 1 byte type + 2 bytes program_id + 2 bytes seq
    REQUIRE(sizeof(beatled_message_program_t) == 5);
  }

  SECTION("Next beat message is 15 bytes (v2)") {
    // base(1) + next_beat_time_ref(8) + beat_count(4) + seq(2) = 15
    REQUIRE(sizeof(beatled_message_next_beat_t) == 15);
  }

  SECTION("Beat message is 15 bytes (v2)") {
    // Same layout as next_beat
    REQUIRE(sizeof(beatled_message_beat_t) == 15);
  }

  SECTION("Message type enum has expected count") {
    // v4 added STATUS_REQUEST + STATUS_RESPONSE
    REQUIRE(BEATLED_MESSAGE_LAST_VALUE == 12);
  }
}
