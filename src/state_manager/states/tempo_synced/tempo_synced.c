#include <stdio.h>

#include "command/command.h"
#include "config/constants.h"
#include "hal/time.h"
#include "state_manager/states/tempo_synced.h"

#define TEMPO_ALARM_DELAY_US 10000000
// Was 100s; with the new median offset filter each sample is cheap and the
// 8-sample ring needs ~30-40s to fill, so a 5s refresh keeps the offset both
// fresh enough to track drift and well-averaged.
#define TIME_ALARM_DELAY_US 5000000
#define HELLO_ALARM_DELAY_US 10000000

static hal_alarm_t *tempo_alarm = NULL;
static hal_alarm_t *time_alarm = NULL;
static hal_alarm_t *hello_alarm = NULL;

static void refresh_tempo_timer_callback(void *data) { send_tempo_request(); }

static void refresh_time_timer_callback(void *data) { send_time_request(); }

static void refresh_hello_timer_callback(void *data) { send_hello_request(); }

void cancel_synced_timers() {
  if (tempo_alarm) {
    hal_cancel_repeating_timer(tempo_alarm);
    tempo_alarm = NULL;
  }
  if (time_alarm) {
    hal_cancel_repeating_timer(time_alarm);
    time_alarm = NULL;
  }
  if (hello_alarm) {
    hal_cancel_repeating_timer(hello_alarm);
    hello_alarm = NULL;
  }
}

int enter_tempo_synced_state() {
  if (tempo_alarm || time_alarm || hello_alarm) {
    BEATLED_FATAL(
        "[ERR] Alarms already active when entering tempo synced state");
    return 1;
  }

  tempo_alarm = hal_add_repeating_timer(TEMPO_ALARM_DELAY_US,
                                        &refresh_tempo_timer_callback, NULL);
  if (!tempo_alarm) {
    puts("[ERR] Failed to allocate tempo alarm");
    return 1;
  }

  time_alarm = hal_add_repeating_timer(TIME_ALARM_DELAY_US,
                                       &refresh_time_timer_callback, NULL);
  if (!time_alarm) {
    puts("[ERR] Failed to allocate time alarm");
    cancel_synced_timers();
    return 1;
  }

  hello_alarm = hal_add_repeating_timer(HELLO_ALARM_DELAY_US,
                                        &refresh_hello_timer_callback, NULL);
  if (!hello_alarm) {
    puts("[ERR] Failed to allocate hello alarm");
    cancel_synced_timers();
    return 1;
  }

  return 0;
}

int exit_tempo_synced_state() {
  cancel_synced_timers();
  return 0;
}