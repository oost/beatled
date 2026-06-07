#include <stdio.h>
#include <stdlib.h>

#include "config/constants.h"
#include "event/event_queue.h"
#include "hal/process.h"
#include "process/core0.h"
#include "state_manager/state_manager.h"

#include "beatled_version.h"
#include "hal/startup.h"

#if defined(PICO_PORT) && defined(FREERTOS_PORT)
#define BEATLED_PORT_NAME "pico-freertos"
#elif defined(PICO_PORT)
#define BEATLED_PORT_NAME "pico"
#elif defined(POSIX_PORT) && defined(FREERTOS_PORT)
#define BEATLED_PORT_NAME "posix-freertos"
#elif defined(POSIX_PORT)
#define BEATLED_PORT_NAME "posix"
#elif defined(ESP32_PORT)
#define BEATLED_PORT_NAME "esp32"
#else
#define BEATLED_PORT_NAME "unknown"
#endif

void start_beatled() {

  printf("[INIT] Starting beatled (port=%s, commit=%s)\n",
         BEATLED_PORT_NAME, BEATLED_GIT_HASH);
  puts("[INIT] Starting state manager");
  state_manager_init();

  if (state_manager_set_state(STATE_STARTED) != 0) {
    return;
  }

  core0_entry(NULL);

  join_cores();
}

#ifndef ESP32_PORT
int main(void) {
  startup(&start_beatled);

  return 0;
}
#endif
