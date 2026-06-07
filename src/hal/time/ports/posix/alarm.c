#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "config/constants.h"
#include "hal/time.h"
struct hal_alarm {
  pthread_t thread_id;
  bool (*timer_delegate)(pthread_t, unsigned int, unsigned int);
  uint64_t useconds;
  alarm_callback_fn callback_fn;
  void *user_data;
};

bool repeating_timer_callback() { return true; }

void *timer_thread_loop(void *data) {

  uint64_t start_time = time_us_64();
  hal_alarm_t *alarm = (hal_alarm_t *)data;
  while (1) {

    // bool should_kill_thread =
    //     alarm->timer_delegate(pthread_self(), start_time, time_us_64());

    // if (should_kill_thread)
    //   pthread_cancel(pthread_self());

    alarm->callback_fn(alarm->user_data);
    usleep(alarm->useconds);
  }
}

hal_alarm_t *hal_add_repeating_timer(int64_t delay_us,
                                     alarm_callback_fn callback_fn,
                                     void *user_data) {
  hal_alarm_t *alarm = (hal_alarm_t *)malloc(sizeof(hal_alarm_t));
  if (!alarm) {
    BEATLED_FATAL("[ERR] Failed to allocate alarm");
  }
  alarm->callback_fn = callback_fn;
  alarm->user_data = user_data;
  alarm->useconds = delay_us;
  // alarm->timer_delegate = should_kill_thread;

  int id = pthread_create(&alarm->thread_id, NULL, timer_thread_loop, alarm);
  if (id) {
    printf("ERROR; return code from pthread_create() is %d\n", id);
    exit(EXIT_FAILURE);
  }

  return alarm;
}

bool hal_cancel_repeating_timer(hal_alarm_t *alarm) {
  pthread_cancel(alarm->thread_id);

  pthread_join(alarm->thread_id, NULL); // blocks main thread
  free(alarm);
  return true;
}
