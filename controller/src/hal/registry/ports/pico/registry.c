#include <pico/sync.h>
#include <string.h>

#include "hal/registry.h"

registry_t registry;
auto_init_mutex(registry_mutex);

void registry_init() {
  memset(&registry, 0, sizeof(registry));
  // registry.tempo_period_us = 120.0 * 1000000 / 60; // Default to 120Hz
  registry.tempo_period_us = 60 * 1000000 / 60; // Default to 0Hz
  registry.program_id = 0;
}

void registry_lock_mutex() { mutex_enter_blocking(&registry_mutex); }

void registry_unlock_mutex() { mutex_exit(&registry_mutex); }

bool registry_try_lock_mutex() {
  return mutex_try_enter(&registry_mutex, NULL);
}