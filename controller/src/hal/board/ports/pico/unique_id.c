#include <pico/unique_id.h>
#include <stdlib.h>
#include <string.h>

#include "hal/unique_id.h"

pico_unique_board_id_t *board_id_internal;

void get_unique_board_id(uint8_t *board_id) {
  pico_get_unique_board_id(board_id_internal);
  memcpy(board_id, board_id_internal->id, BOARD_ID_SIZE_BYTES);
}
