#ifndef COMMAND__QOS_H
#define COMMAND__QOS_H

#include <stdint.h>

#include "beatled/protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

// Bump on any failed intercore_command_queue add. The producer call
// sites in command.c, tempo.c, next_beat.c, and elsewhere call this
// once per drop. Counter saturates at UINT32_MAX (the wire field is
// uint32_t) — wrap is intentional to keep the bump branch-free.
void qos_intercore_drop_bump(void);
uint32_t qos_intercore_drop_total(void);

// Fill an in-memory qos snapshot ready to ship on TEMPO_REQUEST or
// STATUS_RESPONSE. All multi-byte fields are htonl/htonll-encoded so
// the caller can drop the struct straight into the UDP payload.
void qos_block_fill(beatled_qos_block_t *out);

// Test-only reset.
void qos_reset_for_testing(void);

#ifdef __cplusplus
}
#endif

#endif // COMMAND__QOS_H
