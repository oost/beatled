#include <catch2/catch_test_macros.hpp>
#include <utility>

#include "../src/beat_detector/audio/audio_buffer_pool.hpp"
#include "../src/config.hpp"
#include "core/clock.hpp"

using namespace beatled::detector;
using beatled::core::Clock;

TEST_CASE("AudioBufferPools ", "[AudioBufferPool]") {
  AudioBufferPool audio_buffer_pool(beatled::constants::audio_buffer_size,
                                    1000);

  const uint64_t START_TIME = 123456;
  SECTION("Pool can create new AudioBuffers") {
    REQUIRE(audio_buffer_pool.pool_size() == 1);
    REQUIRE(audio_buffer_pool.queue_empty());
    AudioBuffer::Ptr buffer = audio_buffer_pool.get_new_buffer();
    REQUIRE(buffer != nullptr);

    REQUIRE(audio_buffer_pool.pool_empty());

    buffer->set_start_time(START_TIME);
    REQUIRE(buffer->start_time() == START_TIME);

    REQUIRE(buffer->size() == 0);
  }

  SECTION("Pool can enqueue and dequeue buffers") {
    AudioBuffer::Ptr buffer = audio_buffer_pool.get_new_buffer();
    buffer->set_start_time(START_TIME);
    audio_buffer_pool.enqueue(std::move(buffer));
    REQUIRE(buffer == nullptr);
    REQUIRE(!audio_buffer_pool.queue_empty());
    auto new_buffer = audio_buffer_pool.dequeue_blocking();
    REQUIRE(new_buffer->start_time() == START_TIME);
    REQUIRE(audio_buffer_pool.queue_empty());
  }

  SECTION("Buffers can be released back to pool") {
    REQUIRE(audio_buffer_pool.pool_size() == 1);

    auto buffer = audio_buffer_pool.get_new_buffer();
    REQUIRE(audio_buffer_pool.pool_empty());

    buffer->set_start_time(START_TIME);
    REQUIRE(buffer->start_time() == START_TIME);

    audio_buffer_pool.release_buffer(std::move(buffer));
    REQUIRE(!audio_buffer_pool.pool_empty());

    auto buffer2 = audio_buffer_pool.get_new_buffer();
    REQUIRE(audio_buffer_pool.pool_empty());

    uint64_t now = Clock::time_since_epoch_us();
    buffer2->set_start_time(now);
    REQUIRE(buffer2->start_time() == now);
  }
}