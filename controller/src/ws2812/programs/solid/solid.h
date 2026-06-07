#ifndef WS2812__PROGRAMS__SOLID_H
#define WS2812__PROGRAMS__SOLID_H

#include <stddef.h>
#include <stdint.h>

void pattern_solid(uint32_t *stream, size_t len, uint8_t t,
                   uint32_t beat_count);

#endif // WS2812__PROGRAMS__SOLID_H
