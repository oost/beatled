#ifndef WS2812_CONFIG_H
#define WS2812_CONFIG_H

#define IS_RGBW false

#ifndef NUM_PIXELS
#error "NUM_PIXELS is not defined. Pass it at configure time: cmake -DNUM_PIXELS=30 .."
#endif

#ifdef PICO_DEFAULT_WS2812_PIN
#define WS2812_PIN PICO_DEFAULT_WS2812_PIN
#elif !defined(WS2812_PIN)
// default pin — override via cmake -DWS2812_PIN=N or WS2812_PIN env var
#define WS2812_PIN 0
#endif

#endif // WS2812_CONFIG_H