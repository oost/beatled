#include "snake.h"

#include "../utils.h"

static uint8_t position_;
static uint8_t speed_;

void pattern_snakes_init() {
  position_ = 0;
  speed_ = 1;
}

void pattern_snakes(uint32_t *stream, size_t len, uint8_t t,
                    uint32_t beat_count) {
  // Calculate base intensity using quadratic decay for musical beat response
  // Peaks at 255 when t=0 (beat start), decays to 0 when t=255 (beat end)
  uint8_t base_intensity = beat_intensity_quadratic(t) >> 1;  // Right-shift by 1 divides by 2, giving 0-127 range

  // Calculate position for smooth snake movement within each beat
  // Dividing t by 4 slows down movement: 0-255 becomes 0-63
  uint8_t pos = t >> 2;  // Right-shift by 2 divides by 4

  // Rotate hue every 4 beats for visual variety
  uint8_t hue_offset = (beat_count & 0x3) * 64;

  for (unsigned int i = 0; i < len; ++i) {
    unsigned int x = (i + (pos >> 1)) % 64;

    uint32_t value;
    if (x < 10)
      value = convert_hsv_to_rgb(0 + hue_offset, 255, base_intensity);
    else if (x >= 15 && x < 25)
      value = convert_hsv_to_rgb(85 + hue_offset, 255, base_intensity);
    else if (x >= 30 && x < 40)
      value = convert_hsv_to_rgb(170 + hue_offset, 255, base_intensity);
    else
      value = 0;

    stream[i] = value;
  }
}
