#include <stdlib.h>
#include <unistd.h>

#include "hal/unique_id.h"

void get_unique_board_id(uint8_t *board_id) {
  uint32_t pid = (uint32_t)getpid();
  board_id[0] = 0xBE;
  board_id[1] = 0xAD;
  board_id[2] = 'F';
  board_id[3] = 'R';
  board_id[4] = (pid >> 24) & 0xFF;
  board_id[5] = (pid >> 16) & 0xFF;
  board_id[6] = (pid >> 8) & 0xFF;
  board_id[7] = pid & 0xFF;
}

void hal_stdio_init() {}
