// #include "constants.h"
// #include "hal/blink/blink.h"
#include <pico/cyw43_arch.h>

#include "hal/wifi.h"

int wifi_connect(const char *wifi_ssid, const char *wifi_password) {
  if (cyw43_arch_wifi_connect_blocking(wifi_ssid, wifi_password,
                                       CYW43_AUTH_WPA2_AES_PSK)) {
    // blink(ERROR_BLINK_SPEED, ERROR_WIFI);
    puts("[ERR] Failed to connect to WiFi");
    return 1;
  }
  printf("[NET] Connected to %s\n", wifi_ssid);
  // blink(MESSAGE_BLINK_SPEED, MESSAGE_CONNECTED);
  return 0;
}

void wifi_check(const char *wifi_ssid, const char *wifi_password) {
  if (cyw43_wifi_link_status(&cyw43_state, CYW43_ITF_STA) != CYW43_LINK_JOIN) {
    while (1) {
      if (!wifi_connect(wifi_ssid, wifi_password)) {
        return;
      }
    }
  }
}

void hal_wifi_init() {
  if (cyw43_arch_init()) {
    puts("[ERR] WiFi init failed");
    return;
  }
  cyw43_arch_enable_sta_mode();
}

void hal_wifi_deinit() { cyw43_arch_deinit(); }