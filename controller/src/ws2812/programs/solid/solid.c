#include "solid.h"

#include "../utils.h"

// Brightest channel value this program may emit (0-255); bounds current draw.
#define SOLID_MAX_BRIGHTNESS 125u

void pattern_solid(uint32_t *stream, size_t len, uint8_t t, uint32_t beat_count) {
  // Use beat position t (0-255) as perceptual brightness: a ramp from dark
  // at the beat to bright just before the next one. Gamma correction makes
  // the ramp look linear to the eye instead of jumping at the low end.
  uint8_t pos = brightness_apply(t, SOLID_MAX_BRIGHTNESS);

  // Pack brightness into all RGB channels for white/grey color
  // Format: 0xGGRRBB00 (Green=bits 31-24, Red=bits 23-16, Blue=bits 15-8)
  // Multiplying by 0x01010100 sets G=pos, R=pos, B=pos
  for (int i = 0; i < len; ++i) {
    stream[i] = (pos * 0x01010100);
  }
}
