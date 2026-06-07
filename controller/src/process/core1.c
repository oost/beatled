#include <stdio.h>
#include <stdlib.h>

#include "config/constants.h"
#include "hal/process.h"
#include "hal/registry.h"
#include "process/core1.h"
#include "process/intercore_queue.h"
#include "ws2812/ws2812.h"

void *core1_entry(void *data) {
  core1_init();
  core1_loop();
  return NULL;
}

void core1_init() {
  led_init();
  puts("[INIT] Core 1 initialized");
}

void core1_loop() {
  intercore_message_t ic_message;
  puts("[INIT] Core 1 LED loop started");

  uint32_t idx = 0;
  while (1) {
    while (hal_queue_pop_message(intercore_command_queue, &ic_message)) {
      update_tempo(&ic_message);
    }

    led_update();
    sleep_ms(LED_CORE_SLEEP_MS);
    idx++;
  }
}