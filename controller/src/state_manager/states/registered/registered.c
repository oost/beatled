#include <stdio.h>

#include "state_manager/states/registered.h"
#include "command/command.h"
#include "hal/time.h"

// A single TIME_REQUEST is easily lost on Wi-Fi; without a retry the
// controller would wait in REGISTERED forever for a response that will
// never come. Re-send until the transition to TIME_SYNCED cancels the
// timer. 1s is comfortably above any sane RTT and well below the point
// where the wait becomes user-visible.
#define TIME_REQUEST_RETRY_US 1000000

static hal_alarm_t *retry_alarm = NULL;

static void retry_time_request_callback(void *data) {
  printf("[NET] No TIME_RESPONSE yet, re-sending time request\n");
  send_time_request();
}

int enter_registered_state() {
  send_time_request();
  retry_alarm = hal_add_repeating_timer(TIME_REQUEST_RETRY_US, &retry_time_request_callback, NULL);
  if (!retry_alarm) {
    puts("[ERR] Failed to allocate time request retry alarm");
    return 1;
  }
  return 0;
}

int exit_registered_state() {
  if (retry_alarm) {
    hal_cancel_repeating_timer(retry_alarm);
    retry_alarm = NULL;
  }
  return 0;
}
