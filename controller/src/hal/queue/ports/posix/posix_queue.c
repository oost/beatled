#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "circular_buffer.h"
#include "config/constants.h"
#include "hal/queue.h"

#ifndef BEATLED_VERBOSE_LOG
#define BEATLED_VERBOSE_LOG 0
#endif

hal_queue_handle_t hal_queue_init(size_t msg_size, int queue_size) {
  hal_queue_t *new_queue = (hal_queue_t *)malloc(sizeof(hal_queue_t));
  if (!new_queue) {
    BEATLED_FATAL("Failed to allocate queue");
    return NULL;
  }
  queue_init(new_queue, msg_size, queue_size);
  return new_queue;
}

void hal_queue_free(hal_queue_handle_t queue) { queue_free(queue); }

unsigned int hal_queue_size(hal_queue_handle_t queue) {
  return queue_get_level(queue);
}

unsigned int hal_queue_capacity(hal_queue_handle_t queue) {
  return queue_get_capacity(queue);
}

bool hal_queue_add_message(hal_queue_handle_t queue, void *data) {
  bool ok = queue_try_add(queue, data);
#if BEATLED_VERBOSE_LOG
  printf("[QUEUE] add: %s (%u/%u)\n", ok ? "ok" : "FULL",
         queue_get_level(queue), queue_get_capacity(queue));
#endif
  return ok;
}
void hal_queue_add_message_blocking(hal_queue_handle_t queue, void *data) {
  queue_add_blocking(queue, data);
#if BEATLED_VERBOSE_LOG
  printf("[QUEUE] add_blocking: ok (%u/%u)\n", queue_get_level(queue),
         queue_get_capacity(queue));
#endif
}
bool hal_queue_pop_message(hal_queue_handle_t queue, void *data) {
  bool ok = queue_try_remove(queue, data);
  // #if BEATLED_VERBOSE_LOG
  //   printf("[QUEUE] pop: %s (%u/%u)\n", ok ? "ok" : "empty",
  //          queue_get_level(queue), queue_get_capacity(queue));
  // #endif
  return ok;
}
void hal_queue_pop_message_blocking(hal_queue_handle_t queue, void *data) {
  queue_remove_blocking(queue, data);
#if BEATLED_VERBOSE_LOG
  printf("[QUEUE] pop_blocking: ok (%u/%u)\n", queue_get_level(queue),
         queue_get_capacity(queue));
#endif
}
