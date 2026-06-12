#ifndef HAL__WIFI_H
#define HAL__WIFI_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// A single WiFi network to attempt. Empty/NULL ssid marks an unused slot.
typedef struct {
  const char *ssid;
  const char *password;
} wifi_network_t;

void hal_wifi_init();
void hal_wifi_deinit();

// Try each network in order; on a real radio, keep cycling the list until
// one joins. Empty slots (NULL or "" ssid) are skipped.
void wifi_check(const wifi_network_t *networks, size_t count);

#ifdef __cplusplus
}
#endif
#endif // HAL__WIFI_H