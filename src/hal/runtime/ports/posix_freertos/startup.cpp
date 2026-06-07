#include <AppKit/AppKit.hpp>
#include <Metal/Metal.hpp>
#include <MetalKit/MetalKit.hpp>
#include <cstdlib>
#include <thread>

#include "FreeRTOS.h"
#include "task.h"

#include "../posix/app_delegate.h"
#include "hal/startup.h"
#include "../posix/led_buffer.h"
#include "../posix/status_buffer.h"

#define NS_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#define MTK_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION

static startup_main_t startup_main_fn;

static void main_task(void *params) {
  startup_main_fn();
  vTaskDelete(NULL);
}

extern "C" void vApplicationStackOverflowHook(TaskHandle_t xTask,
                                              char *pcTaskName) {
  printf("[FATAL] Stack overflow in task: %s\n", pcTaskName);
  while (1) {
  }
}

void startup(startup_main_t startup_main) {
  startup_main_fn = startup_main;

  // Start FreeRTOS scheduler in a background thread
  std::thread freertos_thread([]() {
    xTaskCreate(main_task, "Main", 4096, NULL, tskIDLE_PRIORITY + 1, NULL);
    vTaskStartScheduler();
  });
  freertos_thread.detach();

  // Metal UI must run on the main thread (macOS requirement)
  NS::AutoreleasePool *pAutoreleasePool = NS::AutoreleasePool::alloc()->init();

  MyAppDelegate del;

  NS::Application *pSharedApplication = NS::Application::sharedApplication();
  pSharedApplication->setDelegate(&del);

  pSharedApplication->run();
  pAutoreleasePool->release();
}

void push_color_stream(uint32_t *stream, uint16_t num_pixel) {
  LEDBuffer::create_instance(stream, num_pixel);
}

void push_status_update(uint8_t state, bool connected, uint16_t program_id,
                        uint32_t tempo_period_us, uint32_t beat_count,
                        int64_t time_offset) {
  StatusBuffer::update(StatusData{state, connected, program_id, tempo_period_us,
                                  beat_count, time_offset});
}
