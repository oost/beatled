#include <catch2/catch_test_macros.hpp>
#include <thread>
#include <utility>
#include <vector>

#include "../src/beat_detector/audio/audio_buffer_pool.hpp"
#include "../src/config.hpp"
#include "core/clock.hpp"

using namespace beatled::detector;
using beatled::core::Clock;

constexpr std::size_t BUF_SIZE = beatled::constants::audio_buffer_size;
constexpr double SAMPLE_RATE = 1000.0;
constexpr std::size_t POOL_CAP = 4;

TEST_CASE("AudioBufferPool pre-allocation", "[AudioBufferPool]") {
  AudioBufferPool pool(BUF_SIZE, SAMPLE_RATE, POOL_CAP);

  SECTION("Pool starts with pre-allocated buffers") {
    REQUIRE(pool.pool_size() == POOL_CAP);
    REQUIRE(pool.total_pool_size() == POOL_CAP);
    REQUIRE(pool.queue_empty());
  }

  SECTION("Getting a buffer reduces pool size") {
    auto buf = pool.get_new_buffer();
    REQUIRE(buf != nullptr);
    REQUIRE(pool.pool_size() == POOL_CAP - 1);
  }

  SECTION("All pre-allocated buffers can be retrieved") {
    std::vector<AudioBuffer::Ptr> buffers;
    for (std::size_t i = 0; i < POOL_CAP; i++) {
      buffers.push_back(pool.get_new_buffer());
      REQUIRE(buffers.back() != nullptr);
    }
    REQUIRE(pool.pool_empty());
  }
}

TEST_CASE("AudioBufferPool enqueue/dequeue", "[AudioBufferPool]") {
  AudioBufferPool pool(BUF_SIZE, SAMPLE_RATE, POOL_CAP);
  const uint64_t START_TIME = 123456;

  SECTION("Pool can enqueue and dequeue buffers") {
    auto buffer = pool.get_new_buffer();
    buffer->set_start_time(START_TIME);
    pool.enqueue(std::move(buffer));
    REQUIRE(buffer == nullptr);
    REQUIRE(!pool.queue_empty());

    auto dequeued = pool.dequeue_blocking();
    REQUIRE(dequeued->start_time() == START_TIME);
    REQUIRE(pool.queue_empty());
  }

  SECTION("Buffer data is preserved through enqueue/dequeue") {
    auto buf = pool.get_new_buffer();
    buf->set_start_time(999999);
    pool.enqueue(std::move(buf));

    auto result = pool.dequeue_blocking();
    REQUIRE(result->start_time() == 999999);
  }
}

TEST_CASE("AudioBufferPool release and reuse", "[AudioBufferPool]") {
  AudioBufferPool pool(BUF_SIZE, SAMPLE_RATE, POOL_CAP);

  SECTION("Buffers can be released back to pool") {
    auto buffer = pool.get_new_buffer();
    REQUIRE(pool.pool_size() == POOL_CAP - 1);

    buffer->set_start_time(123456);
    pool.release_buffer(std::move(buffer));
    REQUIRE(pool.pool_size() == POOL_CAP);

    auto reused = pool.get_new_buffer();
    REQUIRE(reused != nullptr);
    // Buffer should be reset after get_new_buffer
    REQUIRE(reused->size() == 0);
  }

  SECTION("Released buffer can be reused with new data") {
    auto buf = pool.get_new_buffer();
    buf->set_start_time(111);
    pool.release_buffer(std::move(buf));

    auto reused = pool.get_new_buffer();
    uint64_t now = Clock::time_since_epoch_us();
    reused->set_start_time(now);
    REQUIRE(reused->start_time() == now);
  }
}

TEST_CASE("AudioBufferPool exhaustion", "[AudioBufferPool]") {
  AudioBufferPool pool(BUF_SIZE, SAMPLE_RATE, 2);

  SECTION("Exhausted pool allocates emergency buffer after timeout") {
    auto buf1 = pool.get_new_buffer();
    auto buf2 = pool.get_new_buffer();
    REQUIRE(pool.pool_empty());

    // This should wait 100ms and then allocate an emergency buffer
    auto emergency = pool.get_new_buffer();
    REQUIRE(emergency != nullptr);
    REQUIRE(pool.total_pool_size() == 3); // 2 pre-allocated + 1 emergency
  }

  SECTION("Releasing a buffer while another thread waits unblocks it") {
    auto buf1 = pool.get_new_buffer();
    auto buf2 = pool.get_new_buffer();
    REQUIRE(pool.pool_empty());

    AudioBuffer::Ptr result;
    std::thread consumer([&]() { result = pool.get_new_buffer(); });

    // Release a buffer after a short delay so the consumer gets it
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    pool.release_buffer(std::move(buf1));

    consumer.join();
    REQUIRE(result != nullptr);
    // Should not have needed an emergency allocation
    REQUIRE(pool.total_pool_size() == 2);
  }
}

TEST_CASE("AudioBufferPool set_sample_rate", "[AudioBufferPool]") {
  AudioBufferPool pool(BUF_SIZE, SAMPLE_RATE, POOL_CAP);

  SECTION("Changing sample rate rebuilds pool") {
    REQUIRE(pool.pool_size() == POOL_CAP);

    pool.set_sample_rate(48000.0);

    // Pool should be re-filled with new buffers
    REQUIRE(pool.pool_size() == POOL_CAP);
    REQUIRE(pool.total_pool_size() == POOL_CAP);
  }

  SECTION("Buffers after sample rate change are usable") {
    pool.set_sample_rate(44100.0);
    auto buf = pool.get_new_buffer();
    REQUIRE(buf != nullptr);
    buf->set_start_time(555);
    REQUIRE(buf->start_time() == 555);
  }
}

TEST_CASE("AudioBufferPool dequeue_blocking with set_active", "[AudioBufferPool]") {
  AudioBufferPool pool(BUF_SIZE, SAMPLE_RATE, POOL_CAP);

  SECTION("Setting inactive unblocks dequeue") {
    AudioBuffer::Ptr result;
    std::thread consumer([&]() { result = pool.dequeue_blocking(); });

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    pool.set_active(false);

    consumer.join();
    REQUIRE(result == nullptr);
  }
}

TEST_CASE("AudioBufferPool concurrent access", "[AudioBufferPool]") {
  AudioBufferPool pool(BUF_SIZE, SAMPLE_RATE, 4);

  SECTION("Concurrent get/release does not crash") {
    auto worker = [&]() {
      for (int i = 0; i < 50; i++) {
        auto buf = pool.get_new_buffer();
        buf->set_start_time(i);
        pool.release_buffer(std::move(buf));
      }
    };

    std::thread t1(worker);
    std::thread t2(worker);
    std::thread t3(worker);

    t1.join();
    t2.join();
    t3.join();

    // All buffers should be back in pool
    REQUIRE_FALSE(pool.pool_empty());
  }

  SECTION("Concurrent enqueue/dequeue does not crash") {
    constexpr int N = 20;

    auto producer = [&]() {
      for (int i = 0; i < N; i++) {
        auto buf = pool.get_new_buffer();
        buf->set_start_time(i);
        pool.enqueue(std::move(buf));
      }
    };

    auto consumer = [&]() {
      for (int i = 0; i < N; i++) {
        auto buf = pool.dequeue_blocking();
        if (buf) {
          pool.release_buffer(std::move(buf));
        }
      }
    };

    std::thread prod(producer);
    std::thread cons(consumer);

    prod.join();
    cons.join();
  }
}
