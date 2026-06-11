#include <stdio.h>

#include "state_manager/states/time_synced.h"
#include "command/command.h"
#include "hal/time.h"

// Same recovery as REGISTERED's time-request retry: a lost
// TEMPO_RESPONSE must not strand the state machine here.
#define TEMPO_REQUEST_RETRY_US 1000000

static hal_alarm_t *retry_alarm = NULL;

static void retry_tempo_request_callback(void *data) {
  printf("[NET] No TEMPO_RESPONSE yet, re-sending tempo request\n");
  send_tempo_request();
}

int enter_time_synced_state() {
  send_tempo_request();
  retry_alarm =
      hal_add_repeating_timer(TEMPO_REQUEST_RETRY_US, &retry_tempo_request_callback, NULL);
  if (!retry_alarm) {
    puts("[ERR] Failed to allocate tempo request retry alarm");
    return 1;
  }
  return 0;
}

int exit_time_synced_state() {
  if (retry_alarm) {
    hal_cancel_repeating_timer(retry_alarm);
    retry_alarm = NULL;
  }
  return 0;
}
