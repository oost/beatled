#include "greys.h"

#include "../utils.h"

// Brightest channel value this program may emit (0-255); bounds current draw.
#define GREYS_MAX_BRIGHTNESS 32u

void pattern_greys(uint32_t *stream, size_t len, uint8_t t, uint32_t beat_count) {
  // Calculate brightness using 4th-power exponential decay curve
  // t ranges from 0 (beat start) to 255 (beat end)
  // beat_intensity_exp4(0) = 255 (bright flash at beat)
  // beat_intensity_exp4(255) = 0 (faded out at beat end)
  // Gamma-correct the full-range envelope so the fade looks perceptually
  // linear, then cap at the program brightness ceiling.
  uint8_t brightness = brightness_apply(beat_intensity_exp4(t), GREYS_MAX_BRIGHTNESS);

  // Pack RGB values into 32-bit color word
  // Using same brightness for R, G, B creates white/grey color
  uint32_t color = rgb_u32(brightness, brightness, brightness);

  // Set all LEDs in the strip to the same color
  // This creates a synchronized "breathing" effect across the entire strip
  for (int i = 0; i < len; ++i) {
    stream[i] = color; // Write color to LED position i
  }
}
