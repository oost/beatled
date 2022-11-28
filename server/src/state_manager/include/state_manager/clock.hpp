#ifndef STATE_MANAGER__CLOCK_H
#define STATE_MANAGER__CLOCK_H

#include <chrono>
#include <cstdint>

class Clock {
public:
  static uint64_t time_since_epoch_us() {
    return std::chrono::duration_cast<std::chrono::microseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
  }
};

#endif // STATE_MANAGER__CLOCK_H