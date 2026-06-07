#include <stdio.h>
#include <stdlib.h>

#include "autotest.h"
#include "command/command.h"
#include "config/constants.h"
#include "event/event_loop.h"
#include "hal/blink.h"
#include "hal/process.h"
#include "process/core0.h"
#include "state_manager/state_manager.h"

void *core0_entry(void *data) {
  core0_init();
  core0_loop();
  return NULL;
}

void core0_init() {
  puts("[INIT] Core 0 initialized");

#ifdef PICO_AUTOTEST
  puts("[INIT] Running with autotest");
  init_test();
#endif // PICO_AUTOTEST
}

void core0_loop() {
  puts("[INIT] Core 0 event loop started");
  blink(MESSAGE_BLINK_SPEED, MESSAGE_WELCOME);

  event_t event;

#ifdef PICO_AUTOTEST
  test_tempo();
#endif // PICO_AUTOTEST

  run_event_loop(&handle_event);
}
