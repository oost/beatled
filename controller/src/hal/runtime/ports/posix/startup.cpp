#include <AppKit/AppKit.hpp>
#include <Metal/Metal.hpp>
#include <MetalKit/MetalKit.hpp>
#include <cstdlib>
#include <thread>

#include "app_delegate.h"
#include "hal/startup.h"
#include "led_buffer.h"
#include "status_buffer.h"

#define NS_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#define MTK_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION

void startup(startup_main_t startup_main) {

  auto simulator_thread = std::thread{startup_main};

  NS::AutoreleasePool *pAutoreleasePool = NS::AutoreleasePool::alloc()->init();

  MyAppDelegate del;

  NS::Application *pSharedApplication = NS::Application::sharedApplication();
  pSharedApplication->setDelegate(&del);

  pSharedApplication->run();
  pAutoreleasePool->release();

  // simulator_thread.join();
  // exit(EXIT_SUCCESS);
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
