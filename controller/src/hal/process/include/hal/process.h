#ifndef HAL__PROCESS__H
#define HAL__PROCESS__H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

void sleep_ms(uint32_t duration);

typedef void *(*core_loop_fn)(void *);

void start_core1(core_loop_fn core_loop);
void start_isr_thread(core_loop_fn isr_loop);

void join_cores();

#ifdef __cplusplus
}
#endif
#endif // HAL__PROCESS__H