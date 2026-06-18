#ifndef SRC__RUNTIME__INCLUDE__STARTUP__H_
#define SRC__RUNTIME__INCLUDE__STARTUP__H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*startup_main_t)();

void startup(startup_main_t startup_main);

// Simulator HUD hooks. Implemented only by the POSIX renderer; shared code
// reaches them via BEATLED_HUD_UPDATE (config/constants.h), which compiles to
// nothing on hardware, so these are never referenced there.
void push_color_stream(uint32_t *stream, uint16_t num_pixel);
void push_status_update(uint8_t state, bool connected, uint16_t program_id,
                        uint32_t tempo_period_us, uint32_t beat_count, int64_t time_offset);

#ifdef __cplusplus
}
#endif

#endif // SRC__RUNTIME__INCLUDE__STARTUP__H_