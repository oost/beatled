#include "fade.h"

#include <stddef.h>
#include <stdint.h>

#include "../utils.h"

// Brightest channel value these programs may emit (0-255); bounds current draw.
#define FADE_MAX_BRIGHTNESS 64u

void pattern_fade_init() {}

void pattern_fade_grey(uint32_t *stream, size_t len, uint8_t t, uint32_t beat_count) {
  // Quadratic decay for musical beat response: t=0 (beat start) gives max
  // brightness, t=255 (beat end) gives 0. Gamma-correct the full-range
  // envelope, then cap at the program brightness ceiling.
  uint8_t value = brightness_apply(beat_intensity_quadratic(t), FADE_MAX_BRIGHTNESS);

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

void pattern_fade_color(uint32_t *stream, size_t len, uint8_t t, uint32_t beat_count) {
  // Quadratic decay: peaks at beat start (t=0), smoothly fades to 0 by beat
  // end (t=255). Gamma-correct, then cap at the program brightness ceiling.
  uint8_t value = brightness_apply(beat_intensity_quadratic(t), FADE_MAX_BRIGHTNESS);

  // Calculate hue that changes on each beat for color variety
  // beat_count << 2 multiplies by 4: cycles through colors every beat
  // Modulo 255 keeps hue in valid range
  // Full saturation (255) gives vibrant colors; the full-brightness colour
  // is scaled down so dim frames keep a stable hue (see color_scale)
  uint32_t color = color_scale(convert_hsv_to_rgb((beat_count << 2) % 255, 255, 255), value);

  // Set all LEDs to the same fading color
  // Combines beat-synced brightness fade with per-beat color changes
  for (int i = 0; i < len; ++i) {
    stream[i] = color; // Write color to LED position i
  }
}
