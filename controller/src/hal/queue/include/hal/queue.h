#ifndef HAL__QUEUE__QUEUE_H
#define HAL__QUEUE__QUEUE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>

// #include "hal/port/queue.h"

typedef struct hal_queue hal_queue_t;

/// Handle type, the way users interact with the API
typedef hal_queue_t *hal_queue_handle_t;

hal_queue_handle_t hal_queue_init(size_t msg_size, int queue_size);
void hal_queue_free(hal_queue_handle_t queue);

unsigned int hal_queue_size(hal_queue_handle_t queue);
unsigned int hal_queue_capacity(hal_queue_handle_t queue);

bool hal_queue_add_message(hal_queue_handle_t queue, void *data);
void hal_queue_add_message_blocking(hal_queue_handle_t queue, void *data);
bool hal_queue_pop_message(hal_queue_handle_t queue, void *data);
void hal_queue_pop_message_blocking(hal_queue_handle_t queue, void *data);

#ifdef __cplusplus
}
#endif
#endif // HAL__QUEUE__QUEUE_H