#include <stdio.h>

#include "command/command.h"
#include "hal/process.h"
#include "hal/time.h"
#include "process/core0.h"
#include "process/core1.h"
#include "process/isr_thread.h"
#include "state_manager/states/initialized.h"

#define HELLO_ALARM_DELAY_US 10000000

static hal_alarm_t *hello_alarm;

static void hello_timer_callback(void *data) { send_hello_request(); }

int enter_initialized_state() {
  puts("[INIT] Starting core 1");
  start_core1(&core1_entry);

  puts("[INIT] Starting ISR thread");
  start_isr_thread(&isr_thread_entry);

  hello_alarm = hal_add_repeating_timer(HELLO_ALARM_DELAY_US,
                                        &hello_timer_callback, NULL);

  return 0;
}
int exit_initialized_state() {
  hal_cancel_repeating_timer(hello_alarm);
  hello_alarm = NULL;
  return 0;
}
