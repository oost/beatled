#include <stdio.h>
#include <stdlib.h>

#include "FreeRTOS.h"
#include "task.h"

#include "hal/process.h"

static TaskHandle_t core1_task_handle;

void start_core1(core_loop_fn core_loop) {
  xTaskCreate((TaskFunction_t)core_loop, "LED", 2048, NULL,
              tskIDLE_PRIORITY + 2, &core1_task_handle);
  // No core affinity — single core POSIX simulator
}

void start_isr_thread(core_loop_fn isr_loop) {}

void join_cores() {}

// Must provide sleep_ms explicitly — no Pico SDK configSUPPORT_PICO_TIME_INTEROP
void sleep_ms(uint32_t duration) {
  vTaskDelay(pdMS_TO_TICKS(duration));
}
