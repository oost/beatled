#include <catch2/catch_test_macros.hpp>

#include "hal/queue.h"
#include <cstdint>

typedef struct msg {
  int id;
} msg_t;

TEST_CASE("Queue capacity and size", "[queue]") {
  hal_queue_handle_t q = hal_queue_init(sizeof(msg_t), 10);
  REQUIRE(q != nullptr);

  REQUIRE(hal_queue_capacity(q) == 10);
  REQUIRE(hal_queue_size(q) == 0);

  msg_t msg = {.id = 1};
  hal_queue_add_message(q, &msg);
  REQUIRE(hal_queue_size(q) == 1);

  hal_queue_free(q);
}

TEST_CASE("Queue push and pop copies values", "[queue]") {
  hal_queue_handle_t q = hal_queue_init(sizeof(msg_t), 10);

  msg_t in = {.id = 42};
  REQUIRE(hal_queue_add_message(q, &in));

  msg_t out = {.id = 0};
  REQUIRE(hal_queue_pop_message(q, &out));
  REQUIRE(out.id == 42);

  // Modifying the original should not affect what was queued
  in.id = 99;
  hal_queue_add_message(q, &in);
  in.id = 0;
  hal_queue_pop_message(q, &out);
  REQUIRE(out.id == 99);

  hal_queue_free(q);
}

TEST_CASE("Pop from empty queue returns false", "[queue]") {
  hal_queue_handle_t q = hal_queue_init(sizeof(msg_t), 10);

  msg_t out = {.id = 77};
  REQUIRE_FALSE(hal_queue_pop_message(q, &out));
  // Value should not be modified on failed pop
  REQUIRE(out.id == 77);
  REQUIRE(hal_queue_size(q) == 0);

  // Also after adding and removing
  msg_t in = {.id = 1};
  hal_queue_add_message(q, &in);
  hal_queue_pop_message(q, &out);
  out.id = 77;
  REQUIRE_FALSE(hal_queue_pop_message(q, &out));
  REQUIRE(out.id == 77);

  hal_queue_free(q);
}

TEST_CASE("Queue rejects push when full", "[queue]") {
  const int capacity = 4;
  hal_queue_handle_t q = hal_queue_init(sizeof(msg_t), capacity);

  // Fill the queue to capacity
  for (int i = 0; i < capacity; i++) {
    msg_t msg = {.id = i};
    REQUIRE(hal_queue_add_message(q, &msg));
  }
  REQUIRE(hal_queue_size(q) == (unsigned)capacity);

  // Next push should fail
  msg_t overflow = {.id = 999};
  REQUIRE_FALSE(hal_queue_add_message(q, &overflow));
  REQUIRE(hal_queue_size(q) == (unsigned)capacity);

  hal_queue_free(q);
}

TEST_CASE("Queue fill then drain preserves FIFO order", "[queue]") {
  const int capacity = 8;
  hal_queue_handle_t q = hal_queue_init(sizeof(msg_t), capacity);

  // Fill completely
  for (int i = 0; i < capacity; i++) {
    msg_t msg = {.id = i * 10};
    REQUIRE(hal_queue_add_message(q, &msg));
  }
  REQUIRE(hal_queue_size(q) == (unsigned)capacity);

  // Drain completely and verify FIFO order
  for (int i = 0; i < capacity; i++) {
    msg_t out = {.id = -1};
    REQUIRE(hal_queue_pop_message(q, &out));
    REQUIRE(out.id == i * 10);
  }
  REQUIRE(hal_queue_size(q) == 0);

  // Queue should be reusable after drain
  REQUIRE_FALSE(hal_queue_pop_message(q, nullptr));

  hal_queue_free(q);
}

TEST_CASE("Queue wraparound works correctly", "[queue]") {
  const int capacity = 4;
  hal_queue_handle_t q = hal_queue_init(sizeof(msg_t), capacity);

  // Push 3, pop 3 (advances read pointer past halfway)
  for (int i = 0; i < 3; i++) {
    msg_t msg = {.id = i};
    hal_queue_add_message(q, &msg);
  }
  for (int i = 0; i < 3; i++) {
    msg_t out;
    hal_queue_pop_message(q, &out);
    REQUIRE(out.id == i);
  }
  REQUIRE(hal_queue_size(q) == 0);

  // Now push capacity elements -- this forces the write pointer to wrap
  for (int i = 100; i < 100 + capacity; i++) {
    msg_t msg = {.id = i};
    REQUIRE(hal_queue_add_message(q, &msg));
  }
  REQUIRE(hal_queue_size(q) == (unsigned)capacity);

  // Verify FIFO order across the wraparound boundary
  for (int i = 100; i < 100 + capacity; i++) {
    msg_t out;
    REQUIRE(hal_queue_pop_message(q, &out));
    REQUIRE(out.id == i);
  }
  REQUIRE(hal_queue_size(q) == 0);

  hal_queue_free(q);
}

TEST_CASE("Queue interleaved push and pop", "[queue]") {
  const int capacity = 4;
  hal_queue_handle_t q = hal_queue_init(sizeof(msg_t), capacity);
  int next_push = 0;
  int next_pop = 0;

  // Simulate a producer/consumer pattern over many iterations
  // This exercises wraparound repeatedly
  for (int round = 0; round < 20; round++) {
    // Push 2
    for (int i = 0; i < 2; i++) {
      msg_t msg = {.id = next_push++};
      REQUIRE(hal_queue_add_message(q, &msg));
    }
    // Pop 2
    for (int i = 0; i < 2; i++) {
      msg_t out;
      REQUIRE(hal_queue_pop_message(q, &out));
      REQUIRE(out.id == next_pop++);
    }
  }

  REQUIRE(hal_queue_size(q) == 0);
  REQUIRE(next_push == 40);
  REQUIRE(next_pop == 40);

  hal_queue_free(q);
}

TEST_CASE("Queue capacity of 1", "[queue]") {
  hal_queue_handle_t q = hal_queue_init(sizeof(msg_t), 1);
  REQUIRE(hal_queue_capacity(q) == 1);

  msg_t in = {.id = 1};
  REQUIRE(hal_queue_add_message(q, &in));
  REQUIRE(hal_queue_size(q) == 1);

  // Full -- reject second push
  msg_t in2 = {.id = 2};
  REQUIRE_FALSE(hal_queue_add_message(q, &in2));

  // Pop and verify
  msg_t out;
  REQUIRE(hal_queue_pop_message(q, &out));
  REQUIRE(out.id == 1);
  REQUIRE(hal_queue_size(q) == 0);

  // Reuse
  REQUIRE(hal_queue_add_message(q, &in2));
  REQUIRE(hal_queue_pop_message(q, &out));
  REQUIRE(out.id == 2);

  hal_queue_free(q);
}
