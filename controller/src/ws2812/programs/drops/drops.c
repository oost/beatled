#include "drops.h"
#include "../../ws2812_config.h"
#include "../utils.h"

void pattern_drops(uint32_t *stream, size_t len, uint8_t t,
                   uint32_t beat_count) {
  // Calculate drop position that sweeps from start to end of strip within one beat
  // t ranges 0-255, NUM_PIXELS is typically 30
  // Multiply first to avoid losing precision, then divide by 256 (>> 8)
  // Result: t=0 gives pos=0, t=255 gives pos≈NUM_PIXELS
  int pos = (t * NUM_PIXELS) >> 8;  // Map 0-255 beat position to 0-30 LED position

  // Calculate maximum brightness for this beat position using quadratic decay
  // Starts bright at beat (t=0), fades to dark by end (t=255)
  // Right-shift by 2 divides by 4, giving 0-63 range for brightness
  uint8_t max_intensity = beat_intensity_quadratic(t) >> 2;

  // Calculate color for each LED position
  for (int i = 0; i < len; ++i) {
    int value;

    if (i > pos) {
      // LEDs ahead of drop position are dark (drop hasn't reached them yet)
      value = 0;
    } else {
      // LEDs behind or at drop position show trailing gradient
      // Calculate how far behind the drop this LED is
      int distance = pos - i;

      // Trail length is 1/3 of strip length (e.g., 10 LEDs on 30-LED strip)
      int trail_length = NUM_PIXELS / 3;

      if (distance < trail_length) {
        // LED is within trail: calculate gradient brightness
        // At distance=0 (drop position): value = max_intensity
        // At distance=trail_length: value ≈ 0
        // Creates smooth fade from bright to dark
        value = max_intensity - (distance * max_intensity / trail_length);
      } else {
        // LED is too far behind drop, outside trail area
        value = 0;
      }
    }

    // Apply gamma correction for perceptually linear brightness
    // gamma8[] lookup table compensates for human eye's logarithmic response
    value = gamma8[value];

    // Calculate hue that cycles through full color spectrum
    // beat_count << 4 multiplies by 16: changes hue every 16 beats
    // Modulo 256 keeps hue in valid range (0-255)
    uint8_t hue = (beat_count << 4) % 256;

    // Convert HSV (hue, full saturation, calculated brightness) to RGB
    // Saturation=255 gives fully saturated colors (not pastel)
    stream[i] = convert_hsv_to_rgb(hue, 255, value);
  }
}