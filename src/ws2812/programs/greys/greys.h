#ifndef SRC__WS2812__PROGRAMS__GREYS__GREYS__H_
#define SRC__WS2812__PROGRAMS__GREYS__GREYS__H_

#include <stddef.h>
#include <stdint.h>

void pattern_greys(uint32_t *stream, size_t len, uint8_t t,
                   uint32_t beat_count);

#endif // SRC__WS2812__PROGRAMS__GREYS__GREYS__H_