#ifndef BEATLED_CONSTANTS_H
#define BEATLED_CONSTANTS_H

#define SAMPLE_RATE 16000
#define FFT_SIZE 1024
#define INPUT_BUFFER_SIZE 256
#define INPUT_SHIFT 2

#define FFT_BINS_SKIP 5
#define FFT_MAG_MAX 2000.0

#define MAGNITUDE_HISTORY_SIZE 10 * SAMPLE_RATE / INPUT_BUFFER_SIZE
// 10 sec * 16000 samples per sec / 64 samples for every step forward

#define BPM_MIN 80
#define BPM_MAX 200

#define LED_CORE_SLEEP_MS 10
#define CONTROL_CORE_SLEEP_MS 20

// Headroom for the LED loop blocking on output_strings_dma. Each NEXT_BEAT
// now queues a single message (protocol v2 dropped the redundant tempo /
// program bits from the per-beat update), so 128 covers ~minutes of producer
// activity even if the consumer blocks pathologically long. Producers still
// drop-newest on overflow, but at the new rates that case should be rare.
#define MAX_INTERCORE_QUEUE_COUNT 128

// LED messages
#define ERROR_BLINK_SPEED 100
#define MESSAGE_BLINK_SPEED 400

#define ERROR_WIFI 4
#define ERROR_COMMAND 6

#define MESSAGE_WELCOME 2
#define MESSAGE_CONNECTED 3
#define MESSAGE_TIME_UPDATED 4
#define MESSAGE_HELLO 5

#define UDP_SERVER_PORT 9090
#define UDP_PORT 8765

// Verbose logging for [LED], [TEMPO], [QUEUE] debug prints.
// On by default for POSIX development builds, off on Pico.
#ifndef BEATLED_VERBOSE_LOG
#ifdef POSIX_PORT
#define BEATLED_VERBOSE_LOG 1
#else
#define BEATLED_VERBOSE_LOG 0
#endif
#endif

// On POSIX builds, fatal errors abort the program so they're caught
// immediately during development. On ESP32, abort() triggers a core dump
// and reboot. On Pico, just log and continue.
#ifdef POSIX_PORT
#include <stdlib.h>
#define BEATLED_FATAL(msg)                                                                         \
  do {                                                                                             \
    puts("[FATAL] " msg);                                                                          \
    exit(1);                                                                                       \
  } while (0)
#elif defined(ESP32_PORT)
#include <stdlib.h>
#define BEATLED_FATAL(msg)                                                                         \
  do {                                                                                             \
    puts("[FATAL] " msg);                                                                          \
    abort();                                                                                       \
  } while (0)
#else
#define BEATLED_FATAL(msg) puts("[ERR] " msg)
#endif

// Visible R/G/B/W LED bring-up sweep at boot — only meaningful where there's a
// real strip (the Pico builds enable it). A feature flag rather than a port
// check, so the shared LED code carries no #ifdef. Default off; the build
// turns it on.
#ifndef BEATLED_LED_SELF_TEST
#define BEATLED_LED_SELF_TEST 0
#endif

// Whether this build supplies the standard int main() entry point. The ESP-IDF
// build provides its own app_main(), so that build sets this to 0.
#ifndef BEATLED_PROVIDES_MAIN
#define BEATLED_PROVIDES_MAIN 1
#endif

// Simulator HUD hook. The POSIX renderer mirrors controller state to an
// on-screen overlay; real hardware has no HUD, so this compiles to nothing —
// shared code calls it unconditionally and hardware pays no cost (no function
// call). On POSIX it calls push_status_update (declared in hal/startup.h, or
// forward-declared by the caller); callers needn't include any port header.
#ifdef POSIX_PORT
#define BEATLED_HUD_UPDATE(state, connected, program_id, tempo_period_us, beat_count, time_offset) \
  push_status_update((state), (connected), (program_id), (tempo_period_us), (beat_count),          \
                     (time_offset))
#else
#define BEATLED_HUD_UPDATE(state, connected, program_id, tempo_period_us, beat_count, time_offset) \
  do {                                                                                             \
    (void)(state);                                                                                 \
    (void)(connected);                                                                             \
    (void)(program_id);                                                                            \
    (void)(tempo_period_us);                                                                       \
    (void)(beat_count);                                                                            \
    (void)(time_offset);                                                                           \
  } while (0)
#endif

#endif // BEATLED_CONSTANTS_H