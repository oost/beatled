#include <catch2/catch_test_macros.hpp>

typedef struct rgb_color_t {
  uint8_t r;
  uint8_t g;
  uint8_t b;
} rgb_color_t;

static inline rgb_color_t HsvToRgb(uint8_t hue, uint8_t saturation,
                                   uint8_t value) {
  rgb_color_t rgb;
  unsigned char region, remainder, p, q, t;

  if (saturation == 0) {
    rgb.r = value;
    rgb.g = value;
    rgb.b = value;
    return rgb;
  }

  region = hue / 43;
  remainder = (hue - (region * 43)) * 6;

  p = (value * (255 - saturation)) >> 8;
  q = (value * (255 - ((saturation * remainder) >> 8))) >> 8;
  t = (value * (255 - ((saturation * (255 - remainder)) >> 8))) >> 8;

  switch (region) {
  case 0:
    rgb.r = value;
    rgb.g = t;
    rgb.b = p;
    break;
  case 1:
    rgb.r = q;
    rgb.g = value;
    rgb.b = p;
    break;
  case 2:
    rgb.r = p;
    rgb.g = value;
    rgb.b = t;
    break;
  case 3:
    rgb.r = p;
    rgb.g = q;
    rgb.b = value;
    break;
  case 4:
    rgb.r = t;
    rgb.g = p;
    rgb.b = value;
    break;
  default:
    rgb.r = value;
    rgb.g = p;
    rgb.b = q;
    break;
  }

  return rgb;
}

TEST_CASE("HSV to RGB works", "[color]") {
  rgb_color_t rgb = HsvToRgb(0, 255, 255);
  REQUIRE(rgb.r == 255);
  REQUIRE(rgb.g == 0);
  REQUIRE(rgb.b == 0);

  rgb = HsvToRgb(127, 255, 255);
  REQUIRE(rgb.r == 0);
  REQUIRE(rgb.g == 255);
  REQUIRE(rgb.b == 246);

  rgb = HsvToRgb(127, 0, 100);
  REQUIRE(rgb.r == 100);
  REQUIRE(rgb.g == 100);
  REQUIRE(rgb.b == 100);
}