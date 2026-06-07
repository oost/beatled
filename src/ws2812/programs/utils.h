#ifndef SRC__WS2812__PROGRAMS__UTILS__H_
#define SRC__WS2812__PROGRAMS__UTILS__H_

#include <stdint.h>

const static uint8_t gamma8[] = {
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,   1,
    1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   2,   2,   2,   2,
    2,   2,   2,   2,   3,   3,   3,   3,   3,   3,   3,   4,   4,   4,   4,
    4,   5,   5,   5,   5,   6,   6,   6,   6,   7,   7,   7,   7,   8,   8,
    8,   9,   9,   9,   10,  10,  10,  11,  11,  11,  12,  12,  13,  13,  13,
    14,  14,  15,  15,  16,  16,  17,  17,  18,  18,  19,  19,  20,  20,  21,
    21,  22,  22,  23,  24,  24,  25,  25,  26,  27,  27,  28,  29,  29,  30,
    31,  32,  32,  33,  34,  35,  35,  36,  37,  38,  39,  39,  40,  41,  42,
    43,  44,  45,  46,  47,  48,  49,  50,  50,  51,  52,  54,  55,  56,  57,
    58,  59,  60,  61,  62,  63,  64,  66,  67,  68,  69,  70,  72,  73,  74,
    75,  77,  78,  79,  81,  82,  83,  85,  86,  87,  89,  90,  92,  93,  95,
    96,  98,  99,  101, 102, 104, 105, 107, 109, 110, 112, 114, 115, 117, 119,
    120, 122, 124, 126, 127, 129, 131, 133, 135, 137, 138, 140, 142, 144, 146,
    148, 150, 152, 154, 156, 158, 160, 162, 164, 167, 169, 171, 173, 175, 177,
    180, 182, 184, 186, 189, 191, 193, 196, 198, 200, 203, 205, 208, 210, 213,
    215, 218, 220, 223, 225, 228, 231, 233, 236, 239, 241, 244, 247, 249, 252,
    255};

static inline uint32_t rgb_u32(uint8_t r, uint8_t g, uint8_t b) {
  return ((uint32_t)(r) << 16) | ((uint32_t)(g) << 24) | (uint32_t)(b) << 8;
}

// Beat intensity curve functions for musical synchronization
// All functions take t (0-255 beat position) and return intensity (0-255)

// Linear decay: 255 at beat start → 0 at beat end
static inline uint8_t beat_intensity_linear(uint8_t t) {
  return 255 - t;
}

// Quadratic decay: Sharp peak at beat, smooth tail
// Formula: intensity = (1 - (t/255)²) * 255
static inline uint8_t beat_intensity_quadratic(uint8_t t) {
  uint16_t normalized = t;  // 0-255
  uint16_t squared = (normalized * normalized) >> 8;  // (t²/255)
  return 255 - (uint8_t)squared;
}

// Exponential-like decay using 4th power (more dramatic)
// Formula: intensity = (1 - (t/255)⁴) * 255
static inline uint8_t beat_intensity_exp4(uint8_t t) {
  uint32_t n = t;
  uint32_t n2 = (n * n) >> 8;      // t²/255
  uint32_t n4 = (n2 * n2) >> 8;    // t⁴/255²
  return 255 - (uint8_t)n4;
}

// Pulse: Strong flash at beat start, fades quickly in first 25%
static inline uint8_t beat_pulse(uint8_t t) {
  if (t < 64) {  // First quarter of beat
    return 255 - (t << 2);  // Fade from 255→0 over first 64 values
  }
  return 0;  // Off for rest of beat
}

// Strobe: Hard flash at beat start only
static inline uint8_t beat_strobe(uint8_t t) {
  return (t < 16) ? 255 : 0;  // Flash for first ~6% of beat
}

static inline uint32_t convert_hsv_to_rgb(uint8_t hue, uint8_t saturation,
                                          uint8_t value) {
  uint8_t red, green, blue;
  unsigned char region, remainder, p, q, t;

  if (saturation == 0) {
    red = value;
    green = value;
    blue = value;
    return rgb_u32(red, green, blue);
  }

  region = hue / 43;
  remainder = (hue - (region * 43)) * 6;

  p = (value * (255 - saturation)) >> 8;
  q = (value * (255 - ((saturation * remainder) >> 8))) >> 8;
  t = (value * (255 - ((saturation * (255 - remainder)) >> 8))) >> 8;

  switch (region) {
  case 0:
    red = value;
    green = t;
    blue = p;
    break;
  case 1:
    red = q;
    green = value;
    blue = p;
    break;
  case 2:
    red = p;
    green = value;
    blue = t;
    break;
  case 3:
    red = p;
    green = q;
    blue = value;
    break;
  case 4:
    red = t;
    green = p;
    blue = value;
    break;
  default:
    red = value;
    green = p;
    blue = q;
    break;
  }

  return rgb_u32(red, green, blue);
}

#endif // SRC__WS2812__PROGRAMS__UTILS__H_