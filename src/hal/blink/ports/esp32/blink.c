#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#include "hal/blink.h"

#ifndef CONFIG_LED_GPIO
#define CONFIG_LED_GPIO 2
#endif

static bool gpio_initialized = false;

static void ensure_gpio_init(void) {
  if (!gpio_initialized) {
    gpio_reset_pin(CONFIG_LED_GPIO);
    gpio_set_direction(CONFIG_LED_GPIO, GPIO_MODE_OUTPUT);
    gpio_initialized = true;
  }
}

void blink_once(int speed) {
  ensure_gpio_init();
  gpio_set_level(CONFIG_LED_GPIO, 1);
  vTaskDelay(pdMS_TO_TICKS(speed));
  gpio_set_level(CONFIG_LED_GPIO, 0);
}

void blink(int speed, int count) {
  for (int i = 0; i < count; i++) {
    blink_once(speed);
    vTaskDelay(pdMS_TO_TICKS(speed));
  }
}
