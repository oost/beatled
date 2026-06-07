#include <string.h>

#include "esp_mac.h"

#include "hal/unique_id.h"

void get_unique_board_id(uint8_t *board_id) {
  uint8_t mac[6];
  esp_efuse_mac_get_default(mac);
  board_id[0] = 0xBE;
  board_id[1] = 0xAD;
  memcpy(&board_id[2], mac, 6);
}

void hal_stdio_init() {}
