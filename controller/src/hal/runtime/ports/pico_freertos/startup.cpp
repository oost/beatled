#include <stdio.h>

#include <pico/stdio.h>
#include <pico/time.h>

#include "FreeRTOS.h"
#include "task.h"

#include "hal/startup.h"

static startup_main_t startup_main_fn;

static void main_task(void *params) {
  startup_main_fn();
  vTaskDelete(NULL);
}

extern "C" void vApplicationStackOverflowHook(TaskHandle_t xTask,
                                              char *pcTaskName) {
  printf("[FATAL] Stack overflow in task: %s\n", pcTaskName);
  while (1) {
  }
}

void startup(startup_main_t startup_main) {
  // Bring USB CDC up before the scheduler starts; under FreeRTOS the
  // 1ms alarm-driven tud_task() poll is unreliable once the scheduler
  // is masking interrupts during critical sections, so do enumeration
  // while the system is still in a clean state.
  stdio_init_all();
  sleep_ms(2000);

  startup_main_fn = startup_main;
  xTaskCreate(main_task, "Main", 4096, NULL, tskIDLE_PRIORITY + 1, NULL);
  vTaskStartScheduler();
}
