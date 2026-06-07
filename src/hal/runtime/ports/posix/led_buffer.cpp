#include <atomic>

#include "led_buffer.h"
#include "../../../../ws2812/ws2812_config.h"

// Use atomic operations on regular shared_ptr for thread-safe updates between LED
// and render threads (C++11 compatible)
static LEDBuffer::Ptr instance_ =
    std::make_shared<LEDBuffer>(LEDColor{.red = 0, .blue = 1, .green = 0}, NUM_PIXELS);

LEDBuffer::Ptr LEDBuffer::load_instance() {
  return std::atomic_load(&instance_);
}

void LEDBuffer::create_instance(uint32_t *stream, uint16_t num_pixel) {
  auto new_instance = std::make_shared<LEDBuffer>(stream, num_pixel);
  std::atomic_store(&instance_, new_instance);
}