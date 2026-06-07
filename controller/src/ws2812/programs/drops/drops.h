#ifndef SRC__WS2812__PROGRAMS__DROPS__DROPS__H_
#define SRC__WS2812__PROGRAMS__DROPS__DROPS__H_

#include "../../ws2812_patterns.h"

void pattern_drops_init();
void pattern_drops(uint32_t *stream, size_t len, uint8_t t,
                   uint32_t beat_count);

#endif // SRC__WS2812__PROGRAMS__DROPS__DROPS__H_
