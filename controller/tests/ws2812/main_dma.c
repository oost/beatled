/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>

#include "pico/stdlib.h"
#include "pico/sem.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/clocks.h"
#include "ws2812.pio.h"

#define IS_RGBW false

#ifndef WS2812_PIN
#ifdef PICO_DEFAULT_WS2812_PIN
#define WS2812_PIN PICO_DEFAULT_WS2812_PIN
#else
// default to pin 2 if the board doesn't have a default WS2812 pin defined
#define WS2812_PIN 2
#endif
#endif

#if IS_RGBW
#define COLORS_PER_LED 4
#else 
#define COLORS_PER_LED 3
#endif 

#define BYTE_PER_STREAM COLORS_PER_LED * NUM_PIXELS

static uint32_t colors[2][NUM_PIXELS];

absolute_time_t time_ref;
float bpm = 200.0f;

const uint8_t gamma8[] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,
    2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  5,  5,  5,
    5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  9,  9,  9, 10,
   10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
   17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
   25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
   37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
   51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
   69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
   90, 92, 93, 95, 96, 98, 99,101,102,104,105,107,109,110,112,114,
  115,117,119,120,122,124,126,127,129,131,133,135,137,138,140,142,
  144,146,148,150,152,154,156,158,160,162,164,167,169,171,173,175,
  177,180,182,184,186,189,191,193,196,198,200,203,205,208,210,213,
  215,218,220,223,225,228,231,233,236,239,241,244,247,249,252,255 };


static inline uint32_t rgb_u32(uint8_t r, uint8_t g, uint8_t b) {
    return
            ((uint32_t) (r) << 16) |
            ((uint32_t) (g) << 24) |
            (uint32_t) (b) << 8 ;
}

void pattern_snakes(uint32_t *stream, uint len, uint32_t t) {
    uint32_t pos = t >> 26;

    for (uint i = 0; i < len; ++i) {
        uint x = (i + (pos >> 1)) % 64;
        uint32_t value;
        if (x < 10)
            value = (rgb_u32(0xff, 0, 0));
        else if (x >= 15 && x < 25)
            value = (rgb_u32(0, 0xff, 0));
        else if (x >= 30 && x < 40)
            value = (rgb_u32(0, 0, 0xff));
        else
            value = (0);
        stream[i] = value;
    }
}

void pattern_random(uint32_t *stream, uint len, uint32_t t) {
    if ((t >> 29) == 0)
        return;
    for (int i = 0; i < len; ++i)
        stream[i]= (rand());
}

void pattern_sparkle(uint32_t *stream, uint len, uint32_t t) {
    if ((t >> 29) == 0)
        return;
    for (int i = 0; i < len; ++i)
        stream[i] = (rand() % 16 ? 0 : 0xffffffff);
}

void pattern_greys(uint32_t *stream, uint len, uint32_t t) {
    int max = 100; // let's not draw too much current!
    uint8_t brightness = t >> 26;
    for (int i = 0; i < len; ++i) {
        stream[i] = (brightness * 0x10101);
    }
}

void pattern_drops(uint32_t *stream, uint len, uint32_t t) {
    int value;
    int max = (UINT32_MAX / 256);
    int pos = t % NUM_PIXELS;
    int step = max / NUM_PIXELS * 3;
    for( int i = 0; i < len; ++i) {
        if (i > pos) {
            value = 0;
        } else {
            value = (max - (pos-i)*step);
        }
        if (value < 0) { value = 0; }
    
        value = gamma8[value];
        stream[i] = rgb_u32(value, value, value);
    }
}


void pattern_solid(uint32_t *stream, uint len, uint32_t t) {
    uint8_t pos = t >> 24; 
    for (int i = 0; i < len; ++i) {
        stream[i] = (pos * 0x10101);
    }
}

int level = 8;

void pattern_fade(uint32_t *stream, uint len, uint32_t t) {
    uint shift = 4;

    uint max = 16; // let's not draw too much current!
    max <<= shift;

    uint slow_t = t / 32;
    slow_t = level;
    slow_t %= max;

    static int error;
    slow_t += error;
    error = slow_t & ((1u << shift) - 1);
    slow_t >>= shift;
    slow_t *= 0x010101;

    for (int i = 0; i < len; ++i) {
        stream[i] = slow_t;
    }
}

typedef void (*pattern)(uint32_t *stream, uint len, uint32_t t);
const struct {
    pattern pat;
    const char *name;
} pattern_table[] = {
        {pattern_snakes,  "Snakes!"},
        {pattern_random,  "Random data"},
        {pattern_sparkle, "Sparkles"},
        {pattern_greys,   "Greys"},
        {pattern_drops,   "Drops"},
        {pattern_solid,  "Solid!"},
       {pattern_fade, "Fade"},
};


// bit plane content dma channel
#define DMA_CHANNEL 0
// chain channel for configuring main dma channel to output from disjoint 8 word fragments of memory
#define DMA_CB_CHANNEL 1

#define DMA_CHANNEL_MASK (1u << DMA_CHANNEL)
#define DMA_CB_CHANNEL_MASK (1u << DMA_CB_CHANNEL)
#define DMA_CHANNELS_MASK (DMA_CHANNEL_MASK | DMA_CB_CHANNEL_MASK)

// start of each value fragment (+1 for NULL terminator)
static uintptr_t fragment_start[NUM_PIXELS * 4 + 1];

// posted when it is safe to output a new set of values
static struct semaphore reset_delay_complete_sem;
// alarm handle for handling delay
alarm_id_t reset_delay_alarm_id;

int64_t reset_delay_complete(alarm_id_t id, void *user_data) {
    reset_delay_alarm_id = 0;
    sem_release(&reset_delay_complete_sem);
    // no repeat
    // puts("Release semaphore");

    return 0;
}

void __isr dma_complete_handler() {
    if (dma_hw->ints0 & DMA_CHANNEL_MASK) {
        // puts("Interrupt");
        // clear IRQ
        dma_hw->ints0 = DMA_CHANNEL_MASK;
        // when the dma is complete we start the reset delay timer
        if (reset_delay_alarm_id) cancel_alarm(reset_delay_alarm_id);
        reset_delay_alarm_id = add_alarm_in_us(400, reset_delay_complete, NULL, true);
    }
}

void dma_init(PIO pio, uint sm) {
    dma_claim_mask(DMA_CHANNELS_MASK);

    // main DMA channel outputs 8 word fragments, and then chains back to the chain channel
    dma_channel_config channel_config = dma_channel_get_default_config(DMA_CHANNEL);
    channel_config_set_dreq(&channel_config, pio_get_dreq(pio, sm, true));
    // channel_config_set_chain_to(&channel_config, DMA_CB_CHANNEL);
    // channel_config_set_irq_quiet(&channel_config, true);
    dma_channel_configure(DMA_CHANNEL,
                          &channel_config,
                          &pio->txf[sm],
                          NULL, // set by chain
                          NUM_PIXELS, 
                          false);

    irq_set_exclusive_handler(DMA_IRQ_0, dma_complete_handler);
    dma_channel_set_irq0_enabled(DMA_CHANNEL, true);
    irq_set_enabled(DMA_IRQ_0, true);
}

void output_strings_dma(uint32_t *stream) {
    dma_channel_hw_addr(DMA_CHANNEL)->al3_read_addr_trig = (uintptr_t) stream;
}



int main() {
    time_ref = get_absolute_time();	

    //set_sys_clock_48();
    stdio_init_all();
    printf("WS2812 Smoke Test with DMA, using pin %d...\n", WS2812_PIN);

    // todo get free sm
    PIO pio = pio0;
    int sm = 0;
    uint offset = pio_add_program(pio, &ws2812_dma_program);

    ws2812_dma_program_init(pio, sm, offset, WS2812_PIN, 800000, IS_RGBW);

    sem_init(&reset_delay_complete_sem, 1, 1); // initially posted so we don't block first time
    dma_init(pio, sm);
    // const uint LED_PIN = PICO_DEFAULT_LED_PIN;
    // gpio_init(LED_PIN);
    // gpio_set_dir(LED_PIN, GPIO_OUT);

    // int led_on = true;
    uint32_t beat_frac, prev_beat_frac;
    uint32_t beat_us = 60 * 1000000/ bpm;
    int t = 0;
    while (1) {
        // gpio_put(LED_PIN, led_on);
        // led_on = led_on > 0 ?  0 : 1;


        int pat = rand() % count_of(pattern_table);
        int dir = (rand() >> 30) & 1 ? 1 : -1;
        puts(pattern_table[pat].name);
        puts(dir == 1 ? "(forward)" : "(backward)");

        uint current_stream = 0;

        for (int i = 0; i < 1000; ++i) {
            absolute_time_t current_time = get_absolute_time();	
            int64_t duration_from_time_ref =	absolute_time_diff_us(time_ref, current_time);
            beat_frac = (duration_from_time_ref % beat_us) * UINT32_MAX / beat_us;
            if (beat_frac < prev_beat_frac) {
              puts("Beat ... ");
            }
            // printf("Frac %f\n", beat_frac / (1.f * UINT32_MAX));
            pattern_table[pat].pat(colors[current_stream], NUM_PIXELS, beat_frac);

            sem_acquire_blocking(&reset_delay_complete_sem);
            output_strings_dma(colors[current_stream]);
            sleep_ms(20);
            current_stream ^= 1;
            t += dir;
            prev_beat_frac = beat_frac;
        }
    }
}
