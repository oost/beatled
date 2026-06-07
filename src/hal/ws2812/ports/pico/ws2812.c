
#include <hardware/clocks.h>
#include <hardware/dma.h>
#include <hardware/irq.h>
#include <hardware/pio.h>
#include <pico/sem.h>
#include <pico/stdlib.h>
#include <pico/time.h>
#include <stdio.h>
#include <stdlib.h>

#include "hal/ws2812.h"

#include "ws2812.pio.h"
#include "ws2812_dma.h"

static PIO pio;
static uint offset;
static uint sm;
// static uint32_t cycle_idx = 0;
// static uint8_t pattern_idx = 0;

// static uint64_t time_ref = 0;
// static uint32_t tempo_period_us = 120 * 1000000 / 60;

void ws2812_init(uint16_t num_pixel, uint8_t ws2812_pin, uint32_t frequency,
                 bool is_rgbw) {
  // todo get free sm
  pio = pio0;
  offset = pio_add_program(pio, &ws2812_program);

  // Find a free state machine on our chosen PIO (erroring if there are
  // none). Configure it to run our program, and start it, using the
  // helper function we included in our .pio file.
  sm = pio_claim_unused_sm(pio, true);

  ws2812_program_init(pio, sm, offset, ws2812_pin, frequency, is_rgbw);
  dma_init(pio, sm, num_pixel);

  puts("[INIT] WS2812 driver initialized");
}
