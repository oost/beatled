#include <pico/unique_id.h>
#include <stdlib.h>
#include <string.h>

#include "hal/unique_id.h"

void get_unique_board_id(uint8_t *board_id) {
  // Stack-local destination: the previous global-pointer form was
  // never assigned, so pico_get_unique_board_id wrote to NULL and
  // two Picos ended up with identical (all-zero) IDs, causing the
  // server's register_client to dedupe the second one away.
  pico_unique_board_id_t id_local;
  pico_get_unique_board_id(&id_local);
  memcpy(board_id, id_local.id, BOARD_ID_SIZE_BYTES);
}
