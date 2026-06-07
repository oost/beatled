#include <stdio.h>
#include <time.h>

#include "clock/clock.h"
#include "hal/blink.h"
#include "hal/time.h"

// #define SNTP_HOST_NAME "raspberrypi1.local"
// static void sntp_set_system_time_us(sec, us)

// static uint64_t sntp_request_local_time_ref;
// static uint64_t sntp_server_time_ref_us = 0;

static int64_t server_time_offset = 0;

void set_server_time_offset(int64_t new_server_time_offset) {
  server_time_offset = new_server_time_offset;
}

int64_t get_server_time_offset() { return server_time_offset; }

// void sntp_set_system_time(uint32_t sec, uint32_t usec) {
//   puts("Got SNTP response");

//   sntp_request_local_time_ref = get_local_time_us();
//   sntp_server_time_ref_us = sec * 1000000 + usec;

//   // blink(MESSAGE_BLINK_SPEED, MESSAGE_TIME_UPDATED);
//   puts("Got time from SNTP server!");

//   struct tm current_time_val;
//   time_t current_time = (time_t)sec;

// #if defined(_WIN32) || defined(WIN32)
//   localtime_s(&current_time_val, &current_time);
// #else
//   localtime_r(&current_time, &current_time_val);
// #endif

//   char buf[32];

//   strftime(buf, sizeof(buf), "%d.%m.%Y %H:%M:%S", &current_time_val);
//   // LWIP_PLATFORM_DIAG(("SNTP time: %s\n", buf));
// }

bool clock_is_synced() { return server_time_offset != 0; }

// uint64_t get_sntp_server_time_ref_us() { return sntp_server_time_ref_us; }

uint64_t server_time_to_local_time(uint64_t server_time) {
  return (server_time_offset > 0) ? server_time - server_time_offset
                                  : server_time + (-server_time_offset);
  // return server_time + server_time_offset;
  // return delayed_by_us(sntp_request_local_time_ref,
  //                      server_time - sntp_server_time_ref_us);
}
