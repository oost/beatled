#include <atomic>
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <thread>

extern "C" {
#include "hal/time.h"
}

TEST_CASE("Repeating timer", "[alarm]") {

  SECTION("callback is invoked repeatedly") {
    std::atomic<int> count{0};

    auto callback = [](void *user_data) {
      auto *c = static_cast<std::atomic<int> *>(user_data);
      c->fetch_add(1);
    };

    hal_alarm_t *alarm = hal_add_repeating_timer(1000, callback, &count);
    REQUIRE(alarm != nullptr);

    // Let it fire a few times (1ms interval, wait 50ms)
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    hal_cancel_repeating_timer(alarm);

    int final_count = count.load();
    REQUIRE(final_count >= 3);
  }

  SECTION("cancel stops the timer") {
    std::atomic<int> count{0};

    auto callback = [](void *user_data) {
      auto *c = static_cast<std::atomic<int> *>(user_data);
      c->fetch_add(1);
    };

    hal_alarm_t *alarm = hal_add_repeating_timer(1000, callback, &count);
    REQUIRE(alarm != nullptr);

    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    hal_cancel_repeating_timer(alarm);

    int count_at_cancel = count.load();
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    // No more increments after cancel
    REQUIRE(count.load() == count_at_cancel);
  }

  SECTION("user_data is passed to callback") {
    int value = 42;
    std::atomic<bool> received_correct{false};

    struct ctx {
      int *expected;
      std::atomic<bool> *result;
    };
    ctx c{&value, &received_correct};

    auto callback = [](void *user_data) {
      auto *x = static_cast<ctx *>(user_data);
      if (*x->expected == 42) {
        x->result->store(true);
      }
    };

    hal_alarm_t *alarm = hal_add_repeating_timer(1000, callback, &c);
    REQUIRE(alarm != nullptr);

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    hal_cancel_repeating_timer(alarm);

    REQUIRE(received_correct.load());
  }

  SECTION("multiple timers run independently") {
    std::atomic<int> count_a{0};
    std::atomic<int> count_b{0};

    auto callback = [](void *user_data) {
      auto *c = static_cast<std::atomic<int> *>(user_data);
      c->fetch_add(1);
    };

    // Timer A: 1ms, Timer B: 2ms
    hal_alarm_t *a = hal_add_repeating_timer(1000, callback, &count_a);
    hal_alarm_t *b = hal_add_repeating_timer(2000, callback, &count_b);
    REQUIRE(a != nullptr);
    REQUIRE(b != nullptr);

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    hal_cancel_repeating_timer(a);
    hal_cancel_repeating_timer(b);

    // A should have fired roughly twice as often as B
    REQUIRE(count_a.load() > count_b.load());
  }
}
