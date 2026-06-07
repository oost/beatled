#ifndef HAL__TIME_H
#define HAL__TIME_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

uint64_t time_us_64(void);
uint64_t get_local_time_us();

typedef struct hal_alarm hal_alarm_t;
typedef void (*alarm_callback_fn)(void *user_data);

hal_alarm_t *hal_add_alarm(int64_t delay_us, alarm_callback_fn callback_fn,
                           void *user_data);

bool hal_cancel_alarm(hal_alarm_t *alarm);

hal_alarm_t *hal_add_repeating_timer(int64_t delay_us,
                                     alarm_callback_fn callback_fn,
                                     void *user_data);

bool hal_cancel_repeating_timer(hal_alarm_t *alarm);

#ifdef __cplusplus
}
#endif

#endif // HAL__TIME_H