#ifndef SRC__WS2812__PROGRAMS__SPARKLE__SPARKLE__H_
#define SRC__WS2812__PROGRAMS__SPARKLE__SPARKLE__H_

#include <stdint.h>
#include <stdlib.h>

void pattern_sparkle(uint32_t *stream, size_t len, uint8_t t,
                     uint32_t beat_count);

#endif // SRC__WS2812__PROGRAMS__SPARKLE__SPARKLE__H_