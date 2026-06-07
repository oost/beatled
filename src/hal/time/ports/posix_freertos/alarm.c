#include <stdio.h>
#include <stdlib.h>

#ifdef ESP_PLATFORM
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#else
#include "FreeRTOS.h"
#include "timers.h"
#endif

#include "config/constants.h"
#include "hal/time.h"

struct hal_alarm {
  TimerHandle_t timer;
  alarm_callback_fn callback_fn;
  void *user_data;
};

static void timer_callback(TimerHandle_t xTimer) {
  hal_alarm_t *alarm = (hal_alarm_t *)pvTimerGetTimerID(xTimer);
  alarm->callback_fn(alarm->user_data);
}

hal_alarm_t *hal_add_repeating_timer(int64_t delay_us,
                                     alarm_callback_fn callback_fn,
                                     void *user_data) {
  hal_alarm_t *alarm = (hal_alarm_t *)pvPortMalloc(sizeof(hal_alarm_t));
  if (!alarm) {
    BEATLED_FATAL("[ERR] Failed to allocate alarm");
  }
  alarm->callback_fn = callback_fn;
  alarm->user_data = user_data;

  TickType_t period = pdMS_TO_TICKS(delay_us / 1000);
  if (period == 0)
    period = 1;

  alarm->timer =
      xTimerCreate("alarm", period, pdTRUE, alarm, timer_callback);
  if (!alarm->timer) {
    BEATLED_FATAL("[ERR] Failed to create FreeRTOS timer");
  }

  xTimerStart(alarm->timer, portMAX_DELAY);
  return alarm;
}

bool hal_cancel_repeating_timer(hal_alarm_t *alarm) {
  xTimerStop(alarm->timer, portMAX_DELAY);
  xTimerDelete(alarm->timer, portMAX_DELAY);
  vPortFree(alarm);
  return true;
}
