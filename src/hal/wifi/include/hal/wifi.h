#ifndef HAL__WIFI_H
#define HAL__WIFI_H

#ifdef __cplusplus
extern "C" {
#endif

void hal_wifi_init();
void hal_wifi_deinit();

void wifi_check(const char *wifi_ssid, const char *wifi_password);

#ifdef __cplusplus
}
#endif
#endif // HAL__WIFI_H