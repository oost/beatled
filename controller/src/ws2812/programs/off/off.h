#ifndef WS2812__PROGRAMS__OFF_H
#define WS2812__PROGRAMS__OFF_H

#include <stddef.h>
#include <stdint.h>

// Blanks the whole strip: every pixel is set to 0 (LEDs off). Ignores beat
// position and count -- it is a constant black frame.
void pattern_off(uint32_t *stream, size_t len, uint8_t t, uint32_t beat_count);

#endif // WS2812__PROGRAMS__OFF_H
