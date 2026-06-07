#include "greys.h"

#include "../utils.h"

void pattern_greys(uint32_t *stream, size_t len, uint8_t t,
                   uint32_t beat_count) {
  // Calculate brightness using 4th-power exponential decay curve
  // t ranges from 0 (beat start) to 255 (beat end)
  // beat_intensity_exp4(0) = 255 (bright flash at beat)
  // beat_intensity_exp4(255) = 0 (faded out at beat end)
  uint8_t brightness = beat_intensity_exp4(t) >> 3;  // Right-shift by 3 divides by 8, giving 0-31 range

  // Apply gamma correction to compensate for human eye's non-linear brightness perception
  // Without gamma correction, linear fade looks like it drops off too quickly
  // gamma8[] lookup table maps 0-31 to perceptually linear brightness values
  brightness = gamma8[brightness];

  // Pack RGB values into 32-bit color word
  // Using same brightness for R, G, B creates white/grey color
  uint32_t color = rgb_u32(brightness, brightness, brightness);

  // Set all LEDs in the strip to the same color
  // This creates a synchronized "breathing" effect across the entire strip
  for (int i = 0; i < len; ++i) {
    stream[i] = color;  // Write color to LED position i
  }
}