#include <catch2/catch_test_macros.hpp>

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

  SECTION("Hello request includes board_id") {
    // 1 byte type + 17 bytes board_id (2 * 8 + 1)
    REQUIRE(sizeof(beatled_message_hello_request_t) == 18);
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

  SECTION("Tempo request is 5 bytes (v2)") {
    // 1 byte type + 4 bytes owd_us_estimate
    REQUIRE(sizeof(beatled_message_tempo_request_t) == 5);
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
    REQUIRE(BEATLED_MESSAGE_LAST_VALUE == 10);
  }
}
