#include "esp_rom_sys.h"
#include "esp_timer.h"

#include "hal/time.h"

uint64_t time_us_64(void) {
  return (uint64_t)esp_timer_get_time();
}

uint64_t get_local_time_us() {
  return time_us_64();
}

void hal_sleep_ms(uint32_t ms) {
  esp_rom_delay_us((uint32_t)ms * 1000);
}
