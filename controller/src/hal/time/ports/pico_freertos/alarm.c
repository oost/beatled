#include <stdio.h>
#include <stdlib.h>

#include "FreeRTOS.h"
#include "timers.h"

#include "hal/time.h"

// FreeRTOS software timers, not the Pico SDK's add_repeating_timer_us: the
// SDK alarm callbacks run in IRQ context, but every consumer of these
// repeating timers is a network retry that ends in send_udp_request(),
// which takes the lwIP core mutex (LOCK_TCPIP_CORE). Blocking on a mutex
// from an ISR deadlocks the board; software-timer callbacks run in the
// FreeRTOS timer-service task where that's legal. Mirrors the
// posix_freertos port.

struct hal_alarm {
  TimerHandle_t timer;
  alarm_callback_fn callback_fn;
  void *user_data;
};

static void timer_callback(TimerHandle_t xTimer) {
  hal_alarm_t *alarm = (hal_alarm_t *)pvTimerGetTimerID(xTimer);
  alarm->callback_fn(alarm->user_data);
}

hal_alarm_t *hal_add_repeating_timer(int64_t delay_us, alarm_callback_fn callback_fn,
                                     void *user_data) {
  hal_alarm_t *alarm = (hal_alarm_t *)pvPortMalloc(sizeof(hal_alarm_t));
  if (!alarm) {
    puts("[ERR] Failed to allocate alarm");
    return NULL;
  }
  alarm->callback_fn = callback_fn;
  alarm->user_data = user_data;

  TickType_t period = pdMS_TO_TICKS(delay_us / 1000);
  if (period == 0) {
    period = 1;
  }

  alarm->timer = xTimerCreate("alarm", period, pdTRUE, alarm, timer_callback);
  if (!alarm->timer) {
    puts("[ERR] Failed to create FreeRTOS timer");
    vPortFree(alarm);
    return NULL;
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
