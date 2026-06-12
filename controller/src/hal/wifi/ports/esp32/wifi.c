#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"

#include "hal/wifi.h"

static const char *TAG = "wifi";

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

static EventGroupHandle_t wifi_event_group;
static bool wifi_initialized = false;
static int retry_count = 0;
// Per-network attempts before falling through to the next network. Kept
// small so an unreachable AP doesn't hold up the fallback for long.
static const int max_retries = 3;

// Network list to cycle through, owned by the caller of wifi_check().
static const wifi_network_t *s_networks = NULL;
static size_t s_count = 0;
static size_t s_index = 0;

// Returns true if slot i holds a usable network.
static bool network_valid(size_t i) {
  return s_networks != NULL && i < s_count && s_networks[i].ssid != NULL &&
         s_networks[i].ssid[0] != '\0';
}

// Configure the radio for slot i and start connecting. Skips empty slots,
// advancing s_index to the next valid one (wrapping). Returns false only
// when the list holds no usable network at all.
static bool apply_network(size_t i) {
  for (size_t tried = 0; tried < s_count; tried++) {
    size_t idx = (i + tried) % s_count;
    if (!network_valid(idx)) {
      continue;
    }
    s_index = idx;

    wifi_config_t wifi_config = {0};
    strncpy((char *)wifi_config.sta.ssid, s_networks[idx].ssid, sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char *)wifi_config.sta.password, s_networks[idx].password,
            sizeof(wifi_config.sta.password) - 1);
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    printf("[WIFI] Connecting to %s...\n", s_networks[idx].ssid);
    esp_wifi_connect();
    return true;
  }
  return false;
}

static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id,
                          void *event_data) {
  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
    apply_network(s_index);
  } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
    if (retry_count < max_retries) {
      retry_count++;
      ESP_LOGI(TAG, "Retrying %s (%d/%d)", network_valid(s_index) ? s_networks[s_index].ssid : "?",
               retry_count, max_retries);
      esp_wifi_connect();
    } else {
      // Exhausted this network; fall through to the next one and keep
      // cycling forever (never set WIFI_FAIL_BIT).
      retry_count = 0;
      ESP_LOGI(TAG, "Network %s unreachable, trying next",
               network_valid(s_index) ? s_networks[s_index].ssid : "?");
      apply_network(s_index + 1);
    }
  } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
    retry_count = 0;
    xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
  }
}

void hal_wifi_init() {
  if (wifi_initialized)
    return;

  wifi_event_group = xEventGroupCreate();

  esp_netif_init();
  esp_event_loop_create_default();
  esp_netif_create_default_wifi_sta();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&cfg);

  esp_event_handler_instance_t instance_any_id;
  esp_event_handler_instance_t instance_got_ip;
  esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL,
                                      &instance_any_id);
  esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL,
                                      &instance_got_ip);

  esp_wifi_set_mode(WIFI_MODE_STA);

  wifi_initialized = true;
}

void hal_wifi_deinit() {
  if (!wifi_initialized)
    return;
  esp_wifi_stop();
  esp_wifi_deinit();
  wifi_initialized = false;
}

void wifi_check(const wifi_network_t *networks, size_t count) {
  if (!wifi_initialized) {
    puts("[WIFI] Not initialized");
    return;
  }

  s_networks = networks;
  s_count = count;
  s_index = 0;
  retry_count = 0;

  bool any = false;
  for (size_t i = 0; i < count; i++) {
    if (networks[i].ssid != NULL && networks[i].ssid[0] != '\0') {
      s_index = i;
      any = true;
      break;
    }
  }
  if (!any) {
    puts("[WIFI] No networks configured");
    return;
  }

  // The STA_START event kicks off the connection via apply_network(); the
  // disconnect handler cycles through the list until one network joins.
  esp_wifi_start();

  // Block until connected. The handler never gives up (no WIFI_FAIL_BIT),
  // so this waits until some network in the list joins.
  xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
  if (network_valid(s_index)) {
    printf("[WIFI] Connected to %s\n", s_networks[s_index].ssid);
  }
}
