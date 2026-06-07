#ifndef SRC__WS2812__PROGRAMS__RANDOM__RANDOM__H_
#define SRC__WS2812__PROGRAMS__RANDOM__RANDOM__H_

#include <stddef.h>
#include <stdint.h>

void pattern_random(uint32_t *stream, size_t len, uint8_t t,
                    uint32_t beat_count);

#endif // SRC__WS2812__PROGRAMS__RANDOM__RANDOM__H_