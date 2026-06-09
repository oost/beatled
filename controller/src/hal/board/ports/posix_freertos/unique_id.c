#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#if defined(__APPLE__) || defined(__linux__)
#include <sys/random.h> // getentropy
#endif

#include "hal/unique_id.h"

void get_unique_board_id(uint8_t *board_id) {
  // Give each simulator process a stable, unique identity so several instances
  // can register with the server at once. The old PID-based id collided when
  // the OS recycled PIDs across runs; instead draw four random bytes once and
  // cache them. The id must stay constant across the repeated HELLO retries,
  // or the server would see a brand-new device on every packet. Bytes 0..3
  // keep the 0xBE 0xAD "FR" marker that tags POSIX-FreeRTOS builds.
  static uint8_t cached[BOARD_ID_SIZE_BYTES];
  static int initialized = 0;
  if (!initialized) {
    cached[0] = 0xBE;
    cached[1] = 0xAD;
    cached[2] = 'F';
    cached[3] = 'R';
    if (getentropy(&cached[4], 4) != 0) {
      // Fallback if getentropy is somehow unavailable: PID is better than
      // nothing for distinguishing concurrent instances.
      uint32_t pid = (uint32_t)getpid();
      cached[4] = (pid >> 24) & 0xFF;
      cached[5] = (pid >> 16) & 0xFF;
      cached[6] = (pid >> 8) & 0xFF;
      cached[7] = pid & 0xFF;
    }
    initialized = 1;
  }
  memcpy(board_id, cached, BOARD_ID_SIZE_BYTES);
}

void hal_stdio_init() {}
