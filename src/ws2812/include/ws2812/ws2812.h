#ifndef WS2812_H
#define WS2812_H

#include <stdint.h>

#include "process/intercore_queue.h"

void led_init();
void led_set_random_pattern();
void led_update();
void update_tempo(intercore_message_t *ic_message);

uint8_t calculate_beat_fraction(uint64_t current_time, uint64_t last_time,
                                uint64_t next_time);

uint8_t scale8(uint64_t value, uint64_t range);

#endif // WS2812_H