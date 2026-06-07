#include <stdio.h>
#include <stdlib.h>

#include "config/constants.h"
#include "event/event_queue.h"
#include "hal/process.h"
#include "hal/unique_id.h"
#include "process/core0.h"
#include "state_manager/state_manager.h"

#include "beatled_version.h"
#include "config/port_name.h"
#include "hal/startup.h"

void start_beatled() {

  printf("[INIT] Starting beatled (port=%s, commit=%s, built=%s)\n", BEATLED_PORT_NAME,
         BEATLED_GIT_HASH, BEATLED_BUILD_TIME);

  // Print the per-chip board ID at boot so we can verify uniqueness from
  // each Pico's own log, independent of any server-side dedupe. Reads the
  // same `pico_get_unique_board_id` value that gets sent on HELLO_REQUEST.
  uint8_t board_id[BOARD_ID_SIZE_BYTES];
  get_unique_board_id(board_id);
  printf("[INIT] Board ID: ");
  for (size_t i = 0; i < BOARD_ID_SIZE_BYTES; i++) {
    printf("%02x", board_id[i]);
  }
  printf("\n");

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
