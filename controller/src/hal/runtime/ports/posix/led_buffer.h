#ifndef SRC__HAL__RUNTIME__PORTS__POSIX__LED_BUFFER__H_
#define SRC__HAL__RUNTIME__PORTS__POSIX__LED_BUFFER__H_

#include <cstdint>
#include <memory>
#include <vector>

struct LEDColor {
  float blue;
  float red;
  float green;
};

class LEDBuffer {
public:
  using Ptr = std::shared_ptr<LEDBuffer>;
  LEDBuffer(LEDColor color, std::size_t num_led) : stream_(num_led) {
    for (int i = 0; i < num_led; i++) {
      stream_[i] = color;
    }
  }

  LEDBuffer(uint32_t *stream, std::size_t num_led) : stream_(num_led) {
    for (int i = 0; i < num_led; i++) {
      stream_[i] =
          LEDColor{.blue = static_cast<uint8_t>(stream[i] >> 8) / 255.f,
                   .red = static_cast<uint8_t>(stream[i] >> 16) / 255.f,
                   .green = static_cast<uint8_t>(stream[i] >> 24) / 255.f};
    }
  }

  static Ptr load_instance();
  static void create_instance(uint32_t *stream, uint16_t num_pixel);

  const LEDColor &at(std::size_t idx) { return stream_[idx]; }

private:
  std::vector<LEDColor> stream_;
};

#endif // SRC__HAL__RUNTIME__PORTS__POSIX__LED_BUFFER__H_