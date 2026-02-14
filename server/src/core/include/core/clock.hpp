#ifndef CORE__CLOCK_HPP
#define CORE__CLOCK_HPP

#include <chrono>
#include <cstdint>
#include <time.h>

namespace beatled::core {

class Clock : public std::chrono::steady_clock {
public:
  static uint64_t time_since_epoch_us() {
    return std::chrono::duration_cast<std::chrono::microseconds>(
               now().time_since_epoch())
        .count();
  }
  static uint64_t time_us_64() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * (uint64_t)1000000 + ts.tv_nsec / 1000;
  }
  static uint64_t wall_time_us_64() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return ts.tv_sec * (uint64_t)1000000 + ts.tv_nsec / 1000;
  }
};

} // namespace beatled::core

#endif // CORE__CLOCK_HPP