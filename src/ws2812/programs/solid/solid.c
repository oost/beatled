#include "solid.h"

void pattern_solid(uint32_t *stream, size_t len, uint8_t t,
                   uint32_t beat_count) {
  // Use beat position t (0-255) as brightness value
  // Creates a synchronized fade across all LEDs
  uint8_t pos = t;

  // Pack brightness into all RGB channels for white/grey color
  // Format: 0xGGRRBB00 (Green=bits 31-24, Red=bits 23-16, Blue=bits 15-8)
  // Multiplying by 0x01010100 sets G=pos, R=pos, B=pos
  // Result: pos * 0x01010100 = 0x(pos)(pos)(pos)00 = white/grey
  // This creates a simple linear fade synchronized to beat position
  for (int i = 0; i < len; ++i) {
    stream[i] = (pos * 0x01010100);
  }
}
