// #include <pico/stdlib.h>
#include <pico/util/queue.h>
#include <stdio.h>
#include <stdlib.h>

#include "hal/queue.h"

struct hal_queue {
  queue_t impl;
};

hal_queue_handle_t hal_queue_init(size_t msg_size, int queue_size) {
  hal_queue_handle_t new_queue =
      (hal_queue_handle_t)malloc(sizeof(hal_queue_t));
  if (!new_queue) {
    puts("[ERR] Failed to allocate queue");
    return NULL;
  }
  queue_init(&(new_queue->impl), msg_size, queue_size);
  return new_queue;
}

bool hal_queue_add_message(hal_queue_handle_t queue, void *data) {
  if (!queue_try_add(&(queue->impl), data)) {
    puts("[ERR] Queue full");
    return false;
  }
  // puts("Queued message");
  return true;
}

void hal_queue_add_message_blocking(hal_queue_handle_t queue, void *data) {
  queue_add_blocking(&(queue->impl), data);
}

bool hal_queue_pop_message(hal_queue_handle_t queue, void *data) {
  if (!queue_try_remove(&(queue->impl), data)) {
    return false;
  }

  // puts("Popped a message from queue");
  return true;
}

void hal_queue_pop_message_blocking(hal_queue_handle_t queue, void *data) {
  queue_remove_blocking(&(queue->impl), data);
}