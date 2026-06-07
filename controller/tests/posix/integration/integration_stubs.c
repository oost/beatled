/*
 * Integration test stubs — lowest-level HAL replacements only.
 *
 * The real state handlers (started.c, initialized.c, …) are compiled into
 * the test.  They call into HAL functions that would start threads, open
 * sockets, or create timer pthreads.  We replace those here with no-ops
 * that optionally track call counts so the test can assert on them.
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#include "hal/time.h"

/* ── Call counters (reset by the test between cases) ─────────────── */
int stub_send_udp_count = 0;
int stub_timer_create_count = 0;
int stub_timer_cancel_count = 0;

void stub_reset_counters(void) {
  stub_send_udp_count = 0;
  stub_timer_create_count = 0;
  stub_timer_cancel_count = 0;
}

/* ── Time HAL ────────────────────────────────────────────────────────
 * Real clock so that timestamps in events and registry are meaningful.
 * Timer/alarm functions are stubs that return fake pointers.
 */
uint64_t time_us_64(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec * (uint64_t)1000000 + ts.tv_nsec / 1000;
}

uint64_t get_local_time_us(void) { return time_us_64(); }

/* Fake alarm — just a sentinel value so NULL checks pass. */
static char fake_alarm_storage;
static hal_alarm_t *fake_alarm = (hal_alarm_t *)&fake_alarm_storage;

hal_alarm_t *hal_add_alarm(int64_t delay_us, alarm_callback_fn cb,
                           void *user_data) {
  (void)delay_us;
  (void)cb;
  (void)user_data;
  stub_timer_create_count++;
  return fake_alarm;
}

bool hal_cancel_alarm(hal_alarm_t *alarm) {
  (void)alarm;
  stub_timer_cancel_count++;
  return true;
}

hal_alarm_t *hal_add_repeating_timer(int64_t delay_us, alarm_callback_fn cb,
                                     void *user_data) {
  (void)delay_us;
  (void)cb;
  (void)user_data;
  stub_timer_create_count++;
  return fake_alarm;
}

bool hal_cancel_repeating_timer(hal_alarm_t *alarm) {
  (void)alarm;
  stub_timer_cancel_count++;
  return true;
}

/* ── Display / LED stubs ─────────────────────────────────────────── */
void push_status_update(uint8_t state, bool connected, uint16_t program_id,
                        uint32_t tempo_period_us, uint32_t beat_count,
                        int64_t time_offset) {
  (void)state;
  (void)connected;
  (void)program_id;
  (void)tempo_period_us;
  (void)beat_count;
  (void)time_offset;
}

void blink(int speed, int count) {
  (void)speed;
  (void)count;
}
void blink_once(int speed) { (void)speed; }

/* ── Network stubs ───────────────────────────────────────────────── */
typedef int (*prepare_payload_fn)(void *buffer_payload, size_t size);
typedef int (*process_response_fn)(void *buffer_payload, size_t size,
                                   uint64_t rx_time_us);

int send_udp_request(size_t msg_length, prepare_payload_fn prepare_payload) {
  (void)msg_length;
  (void)prepare_payload;
  stub_send_udp_count++;
  return 0;
}

void start_udp(const char *server_name, uint16_t server_port, uint16_t udp_port,
               process_response_fn process_response) {
  (void)server_name;
  (void)server_port;
  (void)udp_port;
  (void)process_response;
}

void shutdown_udp_socket() {}
void udp_print_all_ip_addresses() {}
uint32_t get_ip_address() { return 0; }

/* ── WiFi stubs ──────────────────────────────────────────────────── */
void hal_wifi_init() {}
void hal_wifi_deinit() {}
void wifi_check(const char *ssid, const char *pw) {
  (void)ssid;
  (void)pw;
}
// hal_stdio_init is provided by beatled_hal_board (no-op on posix)

/* ── Process / threading stubs ───────────────────────────────────── */
typedef void *(*core_loop_fn)();

void sleep_ms(uint32_t duration) { (void)duration; }

void start_core1(core_loop_fn core_loop) { (void)core_loop; }
void start_isr_thread(core_loop_fn isr_loop) { (void)isr_loop; }
void join_cores() {}

/* Function pointers passed by initialized.c — never actually called
 * because start_core1 / start_isr_thread are stubs. */
void *core1_entry(void *data) {
  (void)data;
  return NULL;
}
void *isr_thread_entry(void *data) {
  (void)data;
  return NULL;
}
