#include "led_strip.h"

#include "hal/ws2812.h"

static led_strip_handle_t led_strip;
static uint16_t num_pixels_stored;

void ws2812_init(uint16_t num_pixel, uint8_t ws2812_pin, uint32_t frequency,
                 bool is_rgbw) {
  num_pixels_stored = num_pixel;

  led_strip_config_t strip_config = {
      .strip_gpio_num = ws2812_pin,
      .max_leds = num_pixel,
      .led_model = is_rgbw ? LED_MODEL_SK6812 : LED_MODEL_WS2812,
      .color_component_format =
          is_rgbw ? LED_STRIP_COLOR_COMPONENT_FMT_GRBW
                  : LED_STRIP_COLOR_COMPONENT_FMT_GRB,
  };

  led_strip_rmt_config_t rmt_config = {
      .clk_src = RMT_CLK_SRC_DEFAULT,
      .resolution_hz = 10000000, // 10MHz
  };

  led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip);
  led_strip_clear(led_strip);
}

void output_strings_dma(uint32_t *stream) {
  for (uint16_t i = 0; i < num_pixels_stored; i++) {
    uint32_t px = stream[i];
    // Pixel format from rgb_u32(): G<<24 | R<<16 | B<<8
    uint8_t r = (px >> 16) & 0xFF;
    uint8_t g = (px >> 24) & 0xFF;
    uint8_t b = (px >> 8) & 0xFF;
    led_strip_set_pixel(led_strip, i, r, g, b);
  }
  led_strip_refresh(led_strip);
}
