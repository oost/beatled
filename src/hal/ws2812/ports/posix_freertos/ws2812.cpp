#include <cstdbool>
#include <cstdint>

#include "hal/startup.h"
#include "hal/ws2812.h"

static uint16_t num_pixel_ = 1;

void ws2812_init(uint16_t num_pixel, uint8_t ws2812_pin, uint32_t frequency,
                 bool is_rgbw) {
  num_pixel_ = num_pixel;
}

void output_strings_dma(uint32_t *stream) {
  push_color_stream(stream, num_pixel_);
}
