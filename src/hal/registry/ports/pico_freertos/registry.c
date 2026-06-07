#include <string.h>

#ifdef ESP_PLATFORM
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#else
#include "FreeRTOS.h"
#include "semphr.h"
#endif

#include "hal/registry.h"

registry_t registry;
static SemaphoreHandle_t registry_mutex;

void registry_init() {
  registry_mutex = xSemaphoreCreateMutex();
  memset(&registry, 0, sizeof(registry));
  registry.tempo_period_us = 60 * 1000000 / 60;
  registry.program_id = 0;
}

void registry_lock_mutex() {
  xSemaphoreTake(registry_mutex, portMAX_DELAY);
}

void registry_unlock_mutex() { xSemaphoreGive(registry_mutex); }

bool registry_try_lock_mutex() {
  return xSemaphoreTake(registry_mutex, 0) == pdTRUE;
}
