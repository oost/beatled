#ifndef HAL__WS2812_H
#define HAL__WS2812_H

#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>

void ws2812_init(uint16_t num_pixel, uint8_t ws2812_pin, uint32_t frequency,
                 bool is_rgbw);

void output_strings_dma(uint32_t *stream);

#ifdef __cplusplus
}
#endif

#endif // HAL__WS2812_H