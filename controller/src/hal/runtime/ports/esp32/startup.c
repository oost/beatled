#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"

#include "hal/startup.h"

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
  printf("[FATAL] Stack overflow in task: %s\n", pcTaskName);
  abort();
}

void startup(startup_main_t startup_main) {
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    nvs_flash_erase();
    nvs_flash_init();
  }

  // ESP-IDF starts the FreeRTOS scheduler before app_main(),
  // so we just call the main function directly on the main task.
  startup_main();
}
