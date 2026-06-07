---
title: Beatled Protocol
layout: default
parent: Server ↔ Controller Communication
nav_order: 1
---

# Beatled Protocol

Binary UDP protocol for communication between the server (Raspberry Pi 4) and LED controllers (Pico W, ESP32, or POSIX simulator).

## Transport

| Channel | Port | Direction | Description |
|---------|------|-----------|-------------|
| UDP | 9090 | Device → Server | HELLO / TIME / TEMPO requests |
| UDP | 8765 | Server → Devices | NEXT_BEAT / BEAT / PROGRAM / responses |
| HTTPS | 8443 | Client → Server | Web client REST API |

All multi-byte fields are in **network byte order** (big-endian). All structs are packed (`__attribute__((__packed__))`).

Server-side delivery for the per-beat and PROGRAM messages is configurable via `--broadcast-mode={unicast,subnet,limited}`. The default is **unicast** — the server sends one packet per registered client, with NEXT_BEAT timestamps adjusted per-client by that client's measured one-way delay (reported in TEMPO_REQUEST). See [`docs/deployment.markdown`](deployment.html) for when to pick each mode.

## Protocol version

This page documents **protocol v2**. Server and firmware build against a single shared header (`controller/lib/beatled_protocol/include/beatled/protocol.h`); upgrading one without the other is not supported.

## Message Format

Every message starts with a 1-byte type header:

```
┌───────┐
│ type  │  uint8_t — beatled_message_type_t
└───────┘
```

## Message Types

| Type | Value | Direction | Transport |
|------|-------|-----------|-----------|
| ERROR | 0 | Server → Device | Unicast |
| HELLO_REQUEST | 1 | Device → Server | Unicast |
| HELLO_RESPONSE | 2 | Server → Device | Unicast |
| TEMPO_REQUEST | 3 | Device → Server | Unicast |
| TEMPO_RESPONSE | 4 | Server → Device | Unicast |
| TIME_REQUEST | 5 | Device → Server | Unicast |
| TIME_RESPONSE | 6 | Server → Device | Unicast |
| PROGRAM | 7 | Server → Device | Unicast |
| NEXT_BEAT | 8 | Server → Device | Unicast |
| BEAT | 9 | Server → Devices | Broadcast |

---

### ERROR (0)

Sent by the server when a request cannot be processed.

| Offset | Field | Type | Description |
|--------|-------|------|-------------|
| 0 | type | uint8_t | `0` |
| 1 | error_code | uint8_t | Error code (see below) |

**Size**: 2 bytes

**Error codes**:

| Code | Name | Description |
|------|------|-------------|
| 0 | UNKNOWN | Unspecified error |
| 1 | UNKNOWN_MESSAGE_TYPE | Unrecognized message type |
| 2 | NO_DATA | No data available (e.g. no tempo detected yet) |

---

### HELLO_REQUEST (1)

Sent by a controller to register with the server. Contains the device's unique board ID as a hex string.

| Offset | Field | Type | Description |
|--------|-------|------|-------------|
| 0 | type | uint8_t | `1` |
| 1 | board_id | char[17] | Null-terminated hex string (8 bytes × 2 hex chars + NUL) |

**Size**: 18 bytes

---

### HELLO_RESPONSE (2)

Sent by the server to acknowledge registration and assign a client ID.

| Offset | Field | Type | Description |
|--------|-------|------|-------------|
| 0 | type | uint8_t | `2` |
| 1 | client_id | uint16_t | Assigned client identifier |

**Size**: 3 bytes

---

### TEMPO_REQUEST (3)

Sent by a device every ~10 s to refresh the tempo state and report its current one-way-delay estimate.

| Offset | Field | Type | Description |
|--------|-------|------|-------------|
| 0 | type | uint8_t | `3` |
| 1 | owd_us_estimate | uint32_t | Controller's most recent median(RTT)/2 in microseconds; `0` = no sample yet, do not compensate |

**Size**: 5 bytes (was 13 in protocol v1 — the unused beat_time_ref / tempo_period_us echoes were dropped; the OWD field is new)

The server uses `owd_us_estimate` to compensate NEXT_BEAT timestamps per-client. See [Controller Sync](controller-sync.html) for the full round-trip.

---

### TEMPO_RESPONSE (4)

Server response with the current tempo state.

| Offset | Field | Type | Description |
|--------|-------|------|-------------|
| 0 | type | uint8_t | `4` |
| 1 | beat_time_ref | uint64_t | Reference beat timestamp (microseconds since epoch) |
| 9 | tempo_period_us | uint32_t | Beat period in microseconds |
| 13 | program_id | uint16_t | Active LED program ID |

**Size**: 15 bytes

---

### TIME_REQUEST (5)

Sent by a device to initiate NTP-style clock synchronization. The device records its local time as `orig_time` before sending.

| Offset | Field | Type | Description |
|--------|-------|------|-------------|
| 0 | type | uint8_t | `5` |
| 1 | orig_time | uint64_t | Device's local time at send (microseconds) |

**Size**: 9 bytes

---

### TIME_RESPONSE (6)

Server response containing three timestamps for clock offset calculation.

| Offset | Field | Type | Description |
|--------|-------|------|-------------|
| 0 | type | uint8_t | `6` |
| 1 | orig_time | uint64_t | Echoed from request (device's send time) |
| 9 | recv_time | uint64_t | Server's time when request was received |
| 17 | xmit_time | uint64_t | Server's time when response was sent |

**Size**: 25 bytes

#### Clock Offset Calculation

Using the NTP symmetric algorithm:

```
T1 = orig_time    (device send time, device clock)
T2 = recv_time    (server receive time, server clock)
T3 = xmit_time    (server transmit time, server clock)
T4 = local time   (device receive time, device clock)

offset = ((T2 - T1) + (T3 - T4)) / 2
```

The offset is added to device local timestamps to convert them to server time. Multiple rounds of time sync are performed to improve accuracy.

---

### PROGRAM (7)

Sent by the server to change the active LED program on a device. In protocol v2 the server pushes a PROGRAM on every state change *and* a low-rate (~1 Hz) refresh, so late joiners and packet loss don't strand controllers on a wrong pattern. The `seq` field lets controllers ignore stale duplicates and out-of-order pushes.

| Offset | Field | Type | Description |
|--------|-------|------|-------------|
| 0 | type | uint8_t | `7` |
| 1 | program_id | uint16_t | New LED program ID |
| 3 | seq | uint16_t | Monotonically-increasing sequence number (wraps at 65535) |

**Size**: 5 bytes (was 3 in protocol v1 — `seq` was added)

---

### NEXT_BEAT (8)

Sent by the server to each registered device before each beat (unicast by default). Devices use `next_beat_time_ref` to schedule LED updates at the precise moment, and `seq` to detect packet loss.

| Offset | Field | Type | Description |
|--------|-------|------|-------------|
| 0 | type | uint8_t | `8` |
| 1 | next_beat_time_ref | uint64_t | Predicted time of next beat (microseconds, server clock). In unicast mode this value is per-client: the server subtracts the client's `owd_us_estimate` so the embedded time equals the *intended hit instant* on the client's clock when the packet arrives. |
| 9 | beat_count | uint32_t | Running beat counter |
| 13 | seq | uint16_t | Monotonically-increasing sequence number (wraps at 65535); controllers detect loss via gaps and ignore stale duplicates |

**Size**: 15 bytes (was 19 in protocol v1 — `tempo_period_us` and `program_id` were dropped from the per-beat path, and `seq` was added; period now comes only from TEMPO_RESPONSE, program only from PROGRAM)

---

### BEAT (9)

Defined for parity with the beat-detector callback; not currently emitted by the live server. Same shape as NEXT_BEAT.

| Offset | Field | Type | Description |
|--------|-------|------|-------------|
| 0 | type | uint8_t | `9` |
| 1 | beat_time_ref | uint64_t | Time of detected beat (microseconds, server clock) |
| 9 | beat_count | uint32_t | Running beat counter |
| 13 | seq | uint16_t | Monotonically-increasing sequence number |

**Size**: 15 bytes

---

See [Controller Registration and Synchronization](controller-sync.html) for the full startup sequence and state machine walkthrough.
