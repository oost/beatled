#ifndef WS2812_H
#define WS2812_H

#include <stdint.h>

#include "process/intercore_queue.h"

void led_init();
void led_set_random_pattern();
// Render one frame. Returns the suggested sleep before the next frame, in
// microseconds: the regular frame interval, shortened when a beat boundary
// falls inside it so the first frame of every beat lands on the boundary —
// otherwise free-running render loops quantize beat onsets by up to a full
// frame differently on every controller.
uint32_t led_update(void);
void update_tempo(intercore_message_t *ic_message);

uint8_t calculate_beat_fraction(uint64_t current_time, uint64_t last_time, uint64_t next_time);

uint8_t scale8(uint64_t value, uint64_t range);

#endif // WS2812_H