#ifndef CLOCK__CLOCK_H
#define CLOCK__CLOCK_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

// void sntp_sync_init(void);

void set_server_time_offset(int64_t new_server_time_offset);
int64_t get_server_time_offset();
uint64_t server_time_to_local_time(uint64_t server_time);

bool clock_is_synced();
// uint64_t get_sntp_server_time_ref_us();

// void sntp_set_system_time(uint32_t sec, uint32_t nsec);

#ifdef __cplusplus
}
#endif

#endif // CLOCK__CLOCK_H