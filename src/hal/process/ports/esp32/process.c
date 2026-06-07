#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "soc/soc_caps.h"

#include "hal/process.h"

static TaskHandle_t core1_task_handle;
static core_loop_fn core1_fn;
static core_loop_fn isr_fn;

static void core1_wrapper(void *arg) {
  core1_fn(arg);
  vTaskDelete(NULL);
}

static void isr_wrapper(void *arg) {
  isr_fn(arg);
  vTaskDelete(NULL);
}

void start_core1(core_loop_fn core_loop) {
  core1_fn = core_loop;
#if SOC_CPU_CORES_NUM > 1
  xTaskCreatePinnedToCore(core1_wrapper, "LED", 4096, NULL,
                          tskIDLE_PRIORITY + 2, &core1_task_handle, 1);
#else
  xTaskCreate(core1_wrapper, "LED", 4096, NULL,
              tskIDLE_PRIORITY + 2, &core1_task_handle);
#endif
}

void start_isr_thread(core_loop_fn isr_loop) {
  isr_fn = isr_loop;
  xTaskCreate(isr_wrapper, "ISR", 4096, NULL,
              tskIDLE_PRIORITY + 1, NULL);
}

void join_cores() {}

void sleep_ms(uint32_t duration) {
  vTaskDelay(pdMS_TO_TICKS(duration));
}
