#include <pico/cyw43_arch.h>
#include <pico/stdlib.h>

#include "hal/blink.h"

void blink_once(int speed) {
  cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
  sleep_ms(speed);
  cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
  sleep_ms(speed);
}

void blink(int speed, int count) {
  while (count > 0) {
    blink_once(speed);
    count--;
  }
}
