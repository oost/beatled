#include <catch2/catch_test_macros.hpp>
#include <utility>

#include "../src/beat_detector/audio_buffer_pool.hpp"
#include "../src/config.hpp"

using namespace beat_detector;

TEST_CASE("AudioBufferPools ", "[AudioBufferPool]") {
  AudioBufferPool audio_buffer_pool(constants::audio_buffer_size);

  SECTION("Pool can create new AudioBuffers") {
    REQUIRE(audio_buffer_pool.pool_size() == 1);
    REQUIRE(audio_buffer_pool.queue_empty());
    AudioBuffer_ptr buffer = audio_buffer_pool.get_new_buffer();
    REQUIRE(buffer != nullptr);

    REQUIRE(audio_buffer_pool.pool_empty());

    std::chrono::time_point<std::chrono::system_clock> time_point =
        std::chrono::system_clock::now();
    buffer->set_start_time(time_point);
    REQUIRE(buffer->start_time() == time_point);

    REQUIRE(buffer->size() == 0);
  }

  SECTION("Pool can enqueue and dequeue buffers") {
    AudioBuffer_ptr buffer = audio_buffer_pool.get_new_buffer();
    std::chrono::time_point<std::chrono::system_clock> time_point =
        std::chrono::system_clock::now();
    buffer->set_start_time(std::forward<start_time_t>(time_point));
    audio_buffer_pool.enqueue(std::move(buffer));
    REQUIRE(buffer == nullptr);
    REQUIRE(!audio_buffer_pool.queue_empty());
    auto new_buffer = audio_buffer_pool.dequeue_blocking();
    REQUIRE(new_buffer->start_time() == time_point);
    REQUIRE(audio_buffer_pool.queue_empty());
  }

  SECTION("Buffers can be released back to pool") {
    REQUIRE(audio_buffer_pool.pool_size() == 1);

    auto buffer = audio_buffer_pool.get_new_buffer();
    REQUIRE(audio_buffer_pool.pool_empty());

    std::chrono::time_point<std::chrono::system_clock> time_point =
        std::chrono::system_clock::now();
    buffer->set_start_time(time_point);
    REQUIRE(buffer->start_time() == time_point);

    audio_buffer_pool.release_buffer(std::move(buffer));
    REQUIRE(!audio_buffer_pool.pool_empty());

    auto buffer2 = audio_buffer_pool.get_new_buffer();
    REQUIRE(audio_buffer_pool.pool_empty());

    REQUIRE(buffer2->start_time() ==
            std::chrono::time_point<std::chrono::system_clock>());
  }
}