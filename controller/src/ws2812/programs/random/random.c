#include "random.h"

#include <stdlib.h>

#include "../utils.h"

// Brightest channel value this program may emit (0-255); bounds current draw.
#define RANDOM_MAX_BRIGHTNESS 16u

void pattern_random(uint32_t *stream, size_t len, uint8_t t, uint32_t beat_count) {
  // Freeze pattern at the very start of beat for strong visual "hit"
  // t < 8 means first ~3% of beat (8/255 ≈ 0.03)
  if (t < 8) {
    return; // Return early without updating LEDs, keeping previous frame
  }

  // Control update frequency based on position within beat
  // Creates effect: fast chaotic changes early → slow changes later
  uint8_t update_mask;
  if (t < 64) {         // First 25% of beat (64/255 ≈ 0.25)
    update_mask = 0x03; // Binary 0b00000011: update every 4 frames (when t & 0x03 == 0)
  } else if (t < 128) { // Second quarter of beat (25%-50%)
    update_mask = 0x07; // Binary 0b00000111: update every 8 frames (when t & 0x07 == 0)
  } else {              // Second half of beat (50%-100%)
    update_mask = 0x0F; // Binary 0b00001111: update every 16 frames (when t & 0x0F == 0)
  }

  // Check if this frame should update using bitwise AND
  // If result is non-zero, skip this frame (keeps previous colors)
  if ((t & update_mask) != 0) {
    return; // Skip frame, don't update LEDs
  }

  // Calculate maximum brightness using quadratic decay curve
  // Bright at beat start (t=0), dim by beat end (t=255)
  // Gamma-correct the full-range envelope, then cap at the program ceiling
  uint8_t max_brightness = brightness_apply(beat_intensity_quadratic(t), RANDOM_MAX_BRIGHTNESS);

  // Generate random color for each LED
  for (int i = 0; i < len; ++i) {
    // Generate random RGB values between 0 and max_brightness inclusive
    // Each channel is independent, creating random colors
    // The +1 keeps the modulus non-zero when the envelope reaches 0
    uint8_t r = (rand() % (max_brightness + 1u)); // Random red component
    uint8_t g = (rand() % (max_brightness + 1u)); // Random green component
    uint8_t b = (rand() % (max_brightness + 1u)); // Random blue component

    // Pack RGB values into 32-bit color word and write to LED
    stream[i] = rgb_u32(r, g, b);
  }
}