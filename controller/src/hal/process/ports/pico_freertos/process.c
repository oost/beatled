#include <stdio.h>
#include <stdlib.h>

#include "FreeRTOS.h"
#include "task.h"

#include "hal/process.h"

static TaskHandle_t core1_task_handle;

void start_core1(core_loop_fn core_loop) {
  xTaskCreate((TaskFunction_t)core_loop, "LED", 2048, NULL,
              tskIDLE_PRIORITY + 2, &core1_task_handle);
  vTaskCoreAffinitySet(core1_task_handle, (1 << 1));
}

void start_isr_thread(core_loop_fn isr_loop) {}

void join_cores() {}

// sleep_ms is provided by the Pico SDK and is FreeRTOS-aware when
// configSUPPORT_PICO_TIME_INTEROP=1 (set in FreeRTOSConfig.h)
