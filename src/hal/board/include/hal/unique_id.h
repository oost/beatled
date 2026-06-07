#ifndef HAL__UNIQUE_ID_H
#define HAL__UNIQUE_ID_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define BOARD_ID_SIZE_BYTES 8

// typedef struct {
//   uint8_t id[BOARD_ID_SIZE_BYTES];
// } board_id_t;

void get_unique_board_id(uint8_t *board_id);

#ifdef __cplusplus
}
#endif
#endif // HAL__UNIQUE_ID_H