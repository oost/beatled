#include <catch2/catch_all.hpp>
#include <cstdint>

#ifdef __cplusplus
extern "C" {
#endif

#include "beatled/protocol.h"
#include "clock/clock.h"
#include "command/time.h"
#include "event/event_queue.h"
#include "hal/time.h"
#include "ws2812/ws2812.h"

#ifdef __cplusplus
}
#endif

typedef struct msg {
  int id;
} msg_t;

TEST_CASE("Testing clock sync", "[clock]") {
  event_queue_init();
  const int64_t zero_offset = 0;
  const int64_t positive_offset = 12345;
  const int64_t negative_offset = -4321;

  SECTION("Offsets can be saved and retrieved") {
    int64_t server_offset;
    set_server_time_offset(positive_offset);
    server_offset = get_server_time_offset();
    REQUIRE(positive_offset == server_offset);

    set_server_time_offset(negative_offset);
    server_offset = get_server_time_offset();
    REQUIRE(negative_offset == server_offset);
  }

  SECTION("Time can be converted to server time") {
    uint64_t server_time = 12345678;
    uint64_t local_time;

    set_server_time_offset(zero_offset);
    local_time = server_time_to_local_time(server_time);
    REQUIRE(server_time == local_time);

    set_server_time_offset(positive_offset);
    local_time = server_time_to_local_time(server_time);
    REQUIRE(local_time == 12333333);

    set_server_time_offset(negative_offset);
    local_time = server_time_to_local_time(server_time);
    REQUIRE(local_time == 12349999);
  }

  SECTION("Set time offset from server") {
    const uint64_t orig_time = 100;
    const uint64_t server_time = 150;
    const int64_t offset = (int64_t)(server_time) - (int64_t)(orig_time);

    REQUIRE(offset == 50);

    // Protocol v2 rejects TIME_RESPONSE whose echoed orig_time doesn't
    // match the most-recent outstanding request; seed the expected value
    // so this unit test drives the offset path directly.
    time_sync_reset_for_testing();
    time_sync_seed_outstanding_for_testing(orig_time);

    beatled_message_time_response_t resp = {
        .orig_time = htonll(orig_time),
        .recv_time = htonll(orig_time + offset),
        .xmit_time = htonll(orig_time + offset)};
    process_time_msg((beatled_message_t *)(&resp), sizeof(resp), orig_time);
    uint64_t local_time = server_time_to_local_time(server_time);
    REQUIRE(local_time == orig_time);
  }

  SECTION("Test scaling to uin8_t") {
    REQUIRE(scale8(0, 1000) == 0);
    REQUIRE(scale8(500, 1000) == 128);
    REQUIRE(scale8(1000, 1000) == 255);
  }

  SECTION("Test beatled timer function") {
    uint64_t next_beat_time = 1500;
    uint64_t last_beat_time = 500;
    uint64_t current_time = 500;
    uint32_t fraction =
        calculate_beat_fraction(500, last_beat_time, next_beat_time);
    REQUIRE(fraction == 0);
    fraction = calculate_beat_fraction(1500, last_beat_time, next_beat_time);
    REQUIRE(fraction == UINT8_MAX);
    fraction = calculate_beat_fraction(1000, last_beat_time, next_beat_time);
    REQUIRE(fraction == UINT8_MAX / 2 + 1);
  }
}
