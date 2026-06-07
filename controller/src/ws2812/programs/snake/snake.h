#ifndef WS2812__PROGRAMS__SNAKE__SNAKE_H
#define WS2812__PROGRAMS__SNAKE__SNAKE_H

#include "../../ws2812_patterns.h"

void pattern_snakes_init();
void pattern_snakes(uint32_t *stream, size_t len, uint8_t t,
                    uint32_t beat_count);

#endif // WS2812__PROGRAMS__SNAKE__SNAKE_H