// LED pattern behaviour tests, driven through the public run_pattern()
// entry point so the program table stays covered too.

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstring>

#include "beatled/protocol.h"
#include "ws2812_patterns.h"

namespace {

constexpr size_t kLen = 16;

// Program ids from the shared BEATLED_PROGRAM_TABLE.
constexpr int kSnakeId = 0;
constexpr int kOffId = 8;

std::array<uint32_t, kLen> run(int pattern_id, uint8_t beat_pos, uint32_t beat_count) {
  std::array<uint32_t, kLen> stream;
  stream.fill(0xDEADBEEF); // poison so "pattern wrote nothing" can't pass
  run_pattern(pattern_id, stream.data(), stream.size(), beat_pos, beat_count);
  return stream;
}

// The snake body trails behind the head, so the head is the lit pixel
// whose forward neighbour is dark.
size_t find_head(const std::array<uint32_t, kLen> &stream) {
  for (size_t i = 0; i < kLen; ++i) {
    if (stream[i] != 0 && stream[(i + 1) % kLen] == 0) {
      return i;
    }
  }
  FAIL("no snake head found: stream has no lit pixel with a dark successor");
  return 0;
}

} // namespace

TEST_CASE("program table matches the shared protocol header", "[patterns]") {
  REQUIRE(get_pattern_count() == BEATLED_PROGRAM_COUNT);
  CHECK(std::strcmp(pattern_get_name(kSnakeId), "Snakes!") == 0);
  CHECK(std::strcmp(pattern_get_name(kOffId), "Off") == 0);
}

TEST_CASE("off pattern blanks every pixel", "[patterns]") {
  for (uint8_t t : {0, 1, 128, 255}) {
    auto stream = run(kOffId, t, 7);
    for (size_t i = 0; i < kLen; ++i) {
      INFO("t=" << int(t) << " pixel " << i);
      REQUIRE(stream[i] == 0);
    }
  }
}

TEST_CASE("snake head advances one full lap per SNAKE_BEATS_PER_LOOP beats", "[patterns]") {
  // SNAKE_BEATS_PER_LOOP is 4, so at beat boundaries (t=0) the head sits
  // at quarter-lap positions and returns to the start after 4 beats.
  constexpr uint32_t kBeatsPerLoop = 4;
  for (uint32_t beat = 0; beat < 2 * kBeatsPerLoop; ++beat) {
    auto stream = run(kSnakeId, 0, beat);
    size_t expected = (beat % kBeatsPerLoop) * (kLen / kBeatsPerLoop);
    INFO("beat " << beat);
    REQUIRE(find_head(stream) == expected);
  }
}

TEST_CASE("snake head moves within a beat", "[patterns]") {
  // Halfway through beat 0 the head should have advanced half a quarter
  // lap: 128/256 * 16/4 = 2 pixels.
  auto stream = run(kSnakeId, 128, 0);
  REQUIRE(find_head(stream) == 2);
}

TEST_CASE("patterns tolerate len == 0 and out-of-range ids", "[patterns]") {
  // Must not crash or write anywhere.
  run_pattern(kSnakeId, nullptr, 0, 0, 0);
  run_pattern(kOffId, nullptr, 0, 0, 0);

  // run_pattern wraps the id; one full table length past Off is Off again.
  auto stream = run(kOffId + BEATLED_PROGRAM_COUNT, 0, 0);
  for (size_t i = 0; i < kLen; ++i) {
    REQUIRE(stream[i] == 0);
  }
}
