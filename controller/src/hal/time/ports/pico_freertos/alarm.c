#include <pico/stdlib.h>
#include <stdio.h>
#include <stdlib.h>

#include "hal/time.h"

struct hal_alarm {
  repeating_timer_t timer;
  alarm_callback_fn callback_fn;
  void *user_data;
};

bool repeating_timer_callback(struct repeating_timer *t) {
  hal_alarm_t *cb_data = (hal_alarm_t *)t->user_data;
  (cb_data->callback_fn)(cb_data->user_data);
  return true;
}

hal_alarm_t *hal_add_repeating_timer(int64_t delay_us,
                                     alarm_callback_fn callback_fn,
                                     void *user_data) {
  hal_alarm_t *alarm = (hal_alarm_t *)malloc(sizeof(hal_alarm_t));
  if (!alarm) {
    puts("[ERR] Failed to allocate alarm");
    return NULL;
  }
  alarm->callback_fn = callback_fn;
  alarm->user_data = user_data;

  if (!add_repeating_timer_us(delay_us, &repeating_timer_callback, alarm,
                              &alarm->timer)) {
    free(alarm);
    return NULL;
  }
  return alarm;
}

bool hal_cancel_repeating_timer(hal_alarm_t *alarm) {
  bool ret = cancel_repeating_timer(&alarm->timer);
  free(alarm);
  return ret;
}
