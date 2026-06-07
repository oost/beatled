#include <stdio.h>
#include <stdlib.h>

#include "config/constants.h"
#include "event/event_queue.h"
#include "hal/process.h"
#include "process/core0.h"
#include "state_manager/state_manager.h"

#include "beatled_version.h"
#include "config/port_name.h"
#include "hal/startup.h"

void start_beatled() {

  printf("[INIT] Starting beatled (port=%s, commit=%s, built=%s)\n", BEATLED_PORT_NAME,
         BEATLED_GIT_HASH, BEATLED_BUILD_TIME);
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
