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
static const int max_retries = 10;

static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data) {
  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
    esp_wifi_connect();
  } else if (event_base == WIFI_EVENT &&
             event_id == WIFI_EVENT_STA_DISCONNECTED) {
    if (retry_count < max_retries) {
      esp_wifi_connect();
      retry_count++;
      ESP_LOGI(TAG, "Retrying connection (%d/%d)", retry_count, max_retries);
    } else {
      xEventGroupSetBits(wifi_event_group, WIFI_FAIL_BIT);
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
  esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                      &event_handler, NULL,
                                      &instance_any_id);
  esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                      &event_handler, NULL,
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

void wifi_check(const char *wifi_ssid, const char *wifi_password) {
  if (!wifi_initialized) {
    puts("[WIFI] Not initialized");
    return;
  }

  wifi_config_t wifi_config = {0};
  strncpy((char *)wifi_config.sta.ssid, wifi_ssid,
          sizeof(wifi_config.sta.ssid) - 1);
  strncpy((char *)wifi_config.sta.password, wifi_password,
          sizeof(wifi_config.sta.password) - 1);
  wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

  esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
  esp_wifi_start();

  printf("[WIFI] Connecting to %s...\n", wifi_ssid);

  // Block until connected or failed
  EventBits_t bits = xEventGroupWaitBits(wifi_event_group,
                                         WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                         pdFALSE, pdFALSE, portMAX_DELAY);

  if (bits & WIFI_CONNECTED_BIT) {
    printf("[WIFI] Connected to %s\n", wifi_ssid);
  } else if (bits & WIFI_FAIL_BIT) {
    printf("[WIFI] Failed to connect to %s\n", wifi_ssid);
  }
}
