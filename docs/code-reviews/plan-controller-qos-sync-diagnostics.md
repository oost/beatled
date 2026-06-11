# Controller QoS / sync diagnostics

## Context

With the protocol-v3 changes we now know *what firmware* every controller is
running, and with the broadcaster retry + 200 ms refresh we know that
PROGRAM converges quickly. What we still can't see from the server is **how
healthy each link is, how tightly the fleet is synced, and whether any
controller is silently dropping work.** Today every useful metric the
controller computes — median RTT, NEXT_BEAT sequence gaps, time-sync
outliers, intercore queue drops, last-applied PROGRAM seq — stays on the
Pico, and even `owd_us` (which the server already maintains as an EWMA on
`ClientStatus`) never makes it into `/api/devices` JSON.

This plan introduces two complementary diagnostic channels and a small UI
surface so the operator can answer:

- *Are my controllers actually in sync, and by how much do they disagree?*
- *Is any device losing NEXT_BEAT broadcasts, queueing late, or dropping
  PROGRAM pushes?*
- *Did device N really go offline, or is it still alive but quiet?*

Protocol bumps once to v4. Both server and firmware redeploy in lockstep,
same model as v2 and v3.

## Design overview

Three pieces, landed in three commits:

1. **Passive metrics piggy-backed on the existing TEMPO_REQUEST.**
   Free QoS — TEMPO already fires every 10 s while in `TEMPO_SYNCED`.
   Grow the payload from 5 B to ~37 B with a fixed `qos_block_t`:
   median RTT, current clock offset (controller's view), uptime,
   `next_beat_gap_total`, `intercore_drop_total`,
   `time_sync_outlier_total`, `last_applied_program_seq`,
   `valid_sample_count`. Server stores the latest snapshot on
   `ClientStatus`, surfaces in `/api/devices`, and runs aggregation for
   `/api/qos` (new endpoint).

2. **Server-initiated STATUS probe.**
   New `BEATLED_MESSAGE_STATUS_REQUEST` / `_RESPONSE` pair. Server can
   probe a specific client (on demand or on a slow cadence — default 5
   s) without waiting for the controller's 10 s TEMPO heartbeat.
   Carries an echoed server timestamp so the server measures *fresh*
   RTT in addition to harvesting the qos_block. Useful for the UI
   "Inspect" button and for detecting "stale but online" devices.

3. **React QoS surface.**
   Add **Offset**, **RTT**, **Loss** columns to the existing Devices
   table on the Status page; render with sparkline-ish trend cues
   (`12 ms ↑`, `0.3 %`). Add a new **Fleet QoS** card next to
   *Devices*: `max_skew_ms` (max minus min OWD across the fleet), total
   NEXT_BEAT loss this minute, slowest device, green/amber/red health
   pip computed against the configurable thresholds.

## Wire protocol (v4)

Single new struct used as both the trailing block on TEMPO_REQUEST and the
body of STATUS_RESPONSE — so producer / consumer code is shared:

```c
typedef struct {
  int64_t  current_offset_us;     // controller's view of server-time-offset
  uint64_t uptime_us;             // time_us_64() since boot
  uint32_t median_rtt_us;         // already computed in time.c
  uint32_t next_beat_gap_total;   // already exposed via getter
  uint32_t intercore_drop_total;  // NEW — bump three producer sites
  uint32_t time_sync_outlier_total; // NEW — bump in the median filter
  uint16_t valid_sample_count;    // depth of time-sync ring
  uint16_t last_applied_program_seq;
} __attribute__((__packed__)) beatled_qos_block_t;   // 32 bytes
```

```c
typedef struct {                  // TEMPO_REQUEST (was 5 B)
  beatled_message_t base;
  uint32_t owd_us_estimate;       // kept for back-compat-of-intent
  beatled_qos_block_t qos;
} __attribute__((__packed__)) beatled_message_tempo_request_t;  // 37 B

typedef struct {                  // STATUS_REQUEST (server → controller)
  beatled_message_t base;
  uint64_t server_send_time_us;   // echoed back for fresh RTT
} __attribute__((__packed__)) beatled_message_status_request_t; // 9 B

typedef struct {                  // STATUS_RESPONSE (controller → server)
  beatled_message_t base;
  uint64_t echo_server_send_time_us;
  beatled_qos_block_t qos;
} __attribute__((__packed__)) beatled_message_status_response_t; // 41 B
```

All multi-byte fields stay network byte order — reuse the existing
`htonl` / `htonll` (the inline functions we just fixed in
`controller/src/hal/network/include/hal/network.h`) and the same on the
server.

## Controller-side work

Pattern repeats across the four ports, so describe once and list a few
files:

- **New static counters.** Add `static uint32_t intercore_drop_total;`
  and bump it at each `if (!hal_queue_add_message…)` site
  (`controller/src/command/{command,tempo,next_beat,hello,time}.c`).
  Add `static uint32_t time_sync_outlier_total;` and bump in the median
  filter's outlier branch (`controller/src/command/time/time.c`, near
  the existing `median(delay) * 2` threshold check).

- **New getters** (parallel to the existing `next_beat_get_gap_total`):
  `intercore_drop_get_total()`, `time_sync_outlier_get_total()`,
  `time_sync_valid_sample_count()`, `time_sync_median_rtt_us()`,
  `program_get_last_applied_seq()`. Each is a one-line reader; keeps
  the counters file-local.

- **Shared QoS-block builder.** New helper
  `controller/src/command/diagnostics/qos.c` (+ header in
  `controller/src/command/include/command/qos.h`) exposing
  `void qos_block_fill(beatled_qos_block_t *out)` that:
  - reads all the getters above
  - calls `get_server_time_offset()` and `time_us_64()` for offset +
    uptime
  - htonl/htonll-encodes every field
  Both the TEMPO_REQUEST sender and the new STATUS_RESPONSE sender
  call this so the wire form is computed in exactly one place.

- **TEMPO_REQUEST sender.**
  `controller/src/command/tempo/tempo.c::prepare_tempo_request` keeps
  the existing `owd_us_estimate` line and adds
  `qos_block_fill(&msg->qos)`.

- **STATUS receive handler.** Add `process_status_request()` in
  `controller/src/command/status/status.c` (new file):
  - `htonll`-encodes `echo_server_send_time_us` from the request
  - fills the qos block via `qos_block_fill`
  - sends via the existing `send_udp_request` plumbing (new helper
    `send_status_response` matching `send_hello_request` shape)
  Wire it into `controller/src/command/command.c::handle_server_message`
  switch alongside the existing v3 cases. Add the new message type to
  the enum in `controller/lib/beatled_protocol/include/beatled/protocol.h`
  and to `validate_server_message`'s size table.

## Server-side work

- **ClientStatus extension.**
  `server/src/core/include/core/client_status.hpp` gains a single
  `beatled::core::QosSnapshot` value type (mirroring `qos_block_t` plus
  `uint64_t server_received_at_us` and `uint32_t last_rtt_us`). One
  `QosSnapshot latest_qos;` field on `ClientStatus`. `to_json` emits a
  `"qos"` sub-object. The existing `owd_us` EWMA stays — it's the
  server's smoothed view used by the broadcaster.

- **TEMPO_REQUEST handler.**
  `server/src/server/udp_server/udp_request_handler.cpp::process_tempo_request`
  already parses `owd_us_estimate`. Extend it to parse the trailing
  `qos_block_t`, decode via `ntohl`/`ntohll`, and stash into
  `cs->latest_qos`.

- **STATUS probe.**
  - New helper class `StatusProber` in
    `server/src/server/tempo_broadcaster/status_prober.{cpp,hpp}` —
    follows the broadcaster's strand + timer pattern from
    `tempo_broadcaster.cpp::schedule_program_refresh()`. Fires a
    `StatusRequestBuffer` to each registered client every 5 s (CLI
    flag `--status-probe-ms`, default 5000, 0 = off).
  - New `process_status_response` case in the dispatcher's switch
    in `udp_request_handler.cpp::response()`.
  - New `StatusRequestBuffer` and parsing for the response in the
    existing `udp_response_buffer.cpp` / `udp_buffer.hpp` pattern,
    mirroring `TempoResponseBuffer`.
  - Server measures `last_rtt_us = now - echo_server_send_time_us` on
    response.

- **Aggregation + `/api/qos`.**
  Add `QosAggregator` in `server/src/core/qos_aggregator.{cpp,hpp}` —
  pure function over `state_manager_.get_clients()`. Returns
  `{ device_count, max_skew_us, min_offset_us, max_offset_us,
    total_next_beat_loss_per_minute, slowest_device_id,
    unhealthy_count }`. Add `on_get_qos` handler in
  `server/src/server/http/api_handler.cpp` returning the aggregate; add
  the route in `http_server.cpp` next to `/api/devices`.

- **Health thresholds.** Server config gets three CLI flags with sane
  defaults: `--qos-skew-warn-ms=5`, `--qos-skew-fail-ms=20`,
  `--qos-loss-warn-pct=1`. The aggregator uses them to compute the
  health pip.

## React UI work

- **`client/src/lib/status.ts`.** Add `qos` typed field on `Device`
  and a new `getQos()` fetching `/api/qos`. New `FleetQos`
  interface mirroring the aggregator output.

- **`client/src/views/status.tsx`.**
  - Devices table: three new compact columns — Offset (ms),
    RTT (ms), Loss (%) — with green/amber/red text colour driven by
    the same thresholds.
  - New `<FleetQosCard>` next to *Devices*. Renders aggregate
    numbers and a health pip; clicking it expands a compact
    per-device trend mini-table.
  - Status page's existing 2 s `useInterval` covers the new
    fields — no new polling.

- **`client/src/views/__tests__/status.test.tsx`** (new file using the
  pattern already in `program.test.tsx`): loader/action happy + error
  paths with the new `qos` shape mocked at `lib/api`.

## CLI flags / docs

- `--status-probe-ms <ms>` (default 5000, 0 = off)
- `--qos-skew-warn-ms <ms>` (default 5)
- `--qos-skew-fail-ms <ms>` (default 20)
- `--qos-loss-warn-pct <pct>` (default 1)

Document them in `beatled.sh server -h` (the existing Broadcaster /
Server config blocks) and the corresponding tables in
`docs/cli.markdown`. Bump protocol section in
`docs/protocol.md` to v4 with the new sizes + qos_block_t layout.

## Critical files

- `controller/lib/beatled_protocol/include/beatled/protocol.h` — new
  enum values, new structs, `BEATLED_QOS_BLOCK_LEN` constants.
- `controller/src/command/diagnostics/qos.{c,h}` (new) — shared
  qos-block fill helper.
- `controller/src/command/status/status.c` (new) — STATUS handler
  + sender.
- `controller/src/command/{command,tempo,next_beat,time}.c` — counter
  increments + new getters.
- `server/src/core/include/core/client_status.hpp` — `QosSnapshot`
  field + extended `to_json`.
- `server/src/server/udp_server/udp_request_handler.cpp` — parse QoS
  block on TEMPO_REQUEST, handle STATUS_RESPONSE.
- `server/src/server/tempo_broadcaster/status_prober.{cpp,hpp}` (new).
- `server/src/server/udp/udp_response_buffer.{cpp,hpp}` —
  `StatusRequestBuffer`.
- `server/src/core/qos_aggregator.{cpp,hpp}` (new).
- `server/src/server/http/api_handler.{cpp,hpp}` + `http_server.cpp` —
  `/api/qos` route.
- `client/src/lib/status.ts`, `client/src/views/status.tsx`,
  `client/src/views/__tests__/status.test.tsx` (new).

## Verification

- **Unit / catch2:**
  - Extend `server/tests/state_manager/test_client_status_json.cpp` for
    the new `qos` JSON shape.
  - Extend `server/tests/udp/test_udp_request_handler.cpp` to drive a
    TEMPO_REQUEST with a populated qos_block and assert
    `ClientStatus::latest_qos` is correct, plus a STATUS_REQUEST round
    trip.
  - New `server/tests/core/test_qos_aggregator.cpp` covering empty
    fleet, single device, skew computation, threshold transitions.
  - Extend `controller/tests/posix/command/test_command.c` for the
    new qos block builder (fill, htonl round-trip).
  - Bump `controller/tests/posix/protocol/test_protocol.cpp` for the
    new TEMPO_REQUEST + STATUS_* sizes.
- **POSIX firmware integration:**
  - `controller/tests/posix/integration/test_sync_convergence.cpp`
    extends with a "STATUS_REQUEST round-trip" case that asserts the
    responding fields match the controller-side getters' return.
- **End-to-end:**
  - `./beatled.sh server start --start-http --start-udp --start-broadcast`
  - `./beatled.sh controller posix build` from two terminals → two
    fake devices. `curl -k https://localhost:8443/api/qos` returns a
    non-zero `device_count`, sensible aggregates, and (after ~15 s)
    `latest_qos.next_beat_gap_total == 0` per device.
  - Stop one of the simulators; within 5 s a STATUS probe fails for
    it; within 30 s the existing expiry prunes it from `/api/devices`
    but it remains visible as **degraded** until then.
  - Toggle `--qos-skew-fail-ms 0` and watch the pip go red on the
    React Status page.

## Sequencing

Three commits, each independently shippable:

1. *Wire protocol v4 + passive metrics on TEMPO_REQUEST*: counters,
   QoS block, TEMPO_REQUEST extension, `ClientStatus::latest_qos`,
   `/api/devices` JSON, columns in Devices table.
2. *Active STATUS probe + `/api/qos`*: new message types, prober,
   aggregator endpoint, Fleet QoS card.
3. *CLI flags + threshold tuning + docs/cli.markdown +
   docs/protocol.md v4 bump.*
