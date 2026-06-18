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

// Lazily create the mutex so registry_lock_mutex() is safe even when called
// before registry_init() — e.g. unit tests that drive the state machine with
// stubbed enter-handlers that never call registry_init(). FreeRTOS permits
// creating and uncontended-taking a mutex before the scheduler starts, so this
// also removes the old "deadlock taking the not-yet-created mutex" hazard.
static SemaphoreHandle_t ensure_registry_mutex(void) {
  if (registry_mutex == NULL) {
    registry_mutex = xSemaphoreCreateMutex();
  }
  return registry_mutex;
}

void registry_init() {
  ensure_registry_mutex();
  memset(&registry, 0, sizeof(registry));
  registry.tempo_period_us = 60 * 1000000 / 60;
  registry.program_id = 0;
}

void registry_lock_mutex() {
  xSemaphoreTake(ensure_registry_mutex(), portMAX_DELAY);
}

void registry_unlock_mutex() {
  xSemaphoreGive(registry_mutex);
}

bool registry_try_lock_mutex() {
  return xSemaphoreTake(ensure_registry_mutex(), 0) == pdTRUE;
}
