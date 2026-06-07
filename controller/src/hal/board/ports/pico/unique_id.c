#include <pico/unique_id.h>
#include <stdlib.h>
#include <string.h>

#include "hal/unique_id.h"

void get_unique_board_id(uint8_t *board_id) {
  // The Pico SDK fills a caller-owned struct; the previous version of
  // this file declared the destination as a never-initialised global
  // pointer, so pico_get_unique_board_id wrote to NULL and the
  // subsequent memcpy read whatever happened to sit at offset 0 of
  // memory. Two Picos then ended up with identical (all-zero) IDs and
  // the server's register_client deduped the second one away.
  pico_unique_board_id_t id_local;
  pico_get_unique_board_id(&id_local);
  memcpy(board_id, id_local.id, BOARD_ID_SIZE_BYTES);
}
