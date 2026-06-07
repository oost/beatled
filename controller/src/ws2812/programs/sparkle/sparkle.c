#include "sparkle.h"

#include "../utils.h"

void pattern_sparkle(uint32_t *stream, size_t len, uint8_t t,
                     uint32_t beat_count) {
  // Control sparkle density based on position within beat
  // Creates "burst" effect: many sparkles at beat start, then fades to
  // occasional sparkles
  uint8_t density_threshold;
  if (t < 32) {            // First ~12% of beat (t: 0-31)
    density_threshold = 4; // Low threshold = high density (~25% chance per LED)
  } else if (t < 128) {    // First half of beat (t: 32-127)
    density_threshold =
        8; // Medium threshold = medium density (~12% chance per LED)
  } else { // Second half of beat (t: 128-255)
    density_threshold = 16; // High threshold = low density (~6% chance per LED)
  }

  // Calculate brightness using quadratic decay curve for smooth fade
  // Starts at 255 at beat start, decays to 0 by beat end
  uint8_t brightness =
      beat_intensity_quadratic(t) >> 2; // Right-shift by 3 gives 0-31 range

  // Pack brightness into RGB word (white sparkles)
  uint32_t color = rgb_u32(brightness, brightness, brightness);

  // For each LED, randomly decide if it sparkles based on density_threshold
  for (int i = 0; i < len; ++i) {
    // Generate random number mod threshold: if result is 0, LED sparkles
    // Lower threshold = more likely to be 0 = higher sparkle density
    stream[i] = (rand() % density_threshold == 0) ? color : 0;
  }
}
