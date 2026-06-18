#include <pico/time.h>

#include "hal/time.h"

uint64_t get_local_time_us() {
  return time_us_64();
}

void hal_sleep_ms(uint32_t ms) {
  sleep_ms(ms);
}
