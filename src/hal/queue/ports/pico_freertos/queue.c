#include <stdio.h>
#include <stdlib.h>

#ifdef ESP_PLATFORM
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#else
#include "FreeRTOS.h"
#include "queue.h"
#endif

#include "hal/queue.h"

struct hal_queue {
  QueueHandle_t impl;
};

hal_queue_handle_t hal_queue_init(size_t msg_size, int queue_size) {
  hal_queue_handle_t new_queue =
      (hal_queue_handle_t)pvPortMalloc(sizeof(hal_queue_t));
  if (!new_queue) {
    puts("[ERR] Failed to allocate queue");
    return NULL;
  }
  new_queue->impl = xQueueCreate(queue_size, msg_size);
  if (!new_queue->impl) {
    puts("[ERR] Failed to create FreeRTOS queue");
    vPortFree(new_queue);
    return NULL;
  }
  return new_queue;
}

bool hal_queue_add_message(hal_queue_handle_t queue, void *data) {
  if (xQueueSend(queue->impl, data, 0) != pdTRUE) {
    puts("[ERR] Queue full");
    return false;
  }
  return true;
}

void hal_queue_add_message_blocking(hal_queue_handle_t queue, void *data) {
  xQueueSend(queue->impl, data, portMAX_DELAY);
}

bool hal_queue_pop_message(hal_queue_handle_t queue, void *data) {
  return xQueueReceive(queue->impl, data, 0) == pdTRUE;
}

void hal_queue_pop_message_blocking(hal_queue_handle_t queue, void *data) {
  xQueueReceive(queue->impl, data, portMAX_DELAY);
}

void hal_queue_free(hal_queue_handle_t queue) {
  if (queue) {
    vQueueDelete(queue->impl);
    vPortFree(queue);
  }
}

unsigned int hal_queue_size(hal_queue_handle_t queue) {
  return (unsigned int)uxQueueMessagesWaiting(queue->impl);
}

unsigned int hal_queue_capacity(hal_queue_handle_t queue) {
  return (unsigned int)(uxQueueMessagesWaiting(queue->impl) +
                        uxQueueSpacesAvailable(queue->impl));
}
