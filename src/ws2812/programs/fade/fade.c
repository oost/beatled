#include "fade.h"

#include <stddef.h>
#include <stdint.h>

#include "../utils.h"

void pattern_fade_init() {}

void pattern_fade_grey(uint32_t *stream, size_t len, uint8_t t,
                       uint32_t beat_count) {
  // Calculate brightness using quadratic decay for musical beat response
  // t=0 (beat start) gives max brightness, t=255 (beat end) gives 0
  // Right-shift by 2 divides by 16, giving 0-15 range (conservative brightness)
  uint8_t value = beat_intensity_quadratic(t) >> 2;

  // Pack the same brightness value into R, G, B channels to create grey/white
  // 0x01010100 is bit pattern with 1s in R, G, B byte positions
  // Multiplying by value sets all three channels to same brightness
  uint32_t v = value * 0x01010100;

  // Set all LEDs to the same grey value
  // Creates synchronized fade across entire strip
  for (int i = 0; i < len; ++i) {
    stream[i] = v; // Write grey color to LED position i
  }
}

void pattern_fade_color(uint32_t *stream, size_t len, uint8_t t,
                        uint32_t beat_count) {
  // Calculate brightness value using quadratic decay curve
  // Peaks at beat start (t=0), smoothly fades to 0 by beat end (t=255)
  // Right-shift by 2 divides by 16, giving 0-15 range for gentle fade
  uint8_t value = beat_intensity_quadratic(t) >> 2;

  // Calculate hue that changes on each beat for color variety
  // beat_count << 2 multiplies by 4: cycles through colors every beat
  // Modulo 255 keeps hue in valid range
  // Full saturation (255) gives vibrant colors
  // 'value' controls brightness (V in HSV), synchronized to beat
  uint32_t color = convert_hsv_to_rgb((beat_count << 2) % 255, 255, value);

  // Set all LEDs to the same fading color
  // Combines beat-synced brightness fade with per-beat color changes
  for (int i = 0; i < len; ++i) {
    stream[i] = color; // Write color to LED position i
  }
}
