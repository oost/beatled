#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>

void vApplicationStackOverflowHook(TaskHandle_t xTask,
                                   char *pcTaskName) {
  printf("[TEST] Stack overflow in task: %s\n", pcTaskName);
}
