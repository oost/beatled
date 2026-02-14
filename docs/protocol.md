---
title: Beatled Protocol
layout: default
nav_order: 6
---

# Beatled Protocol

Binary UDP protocol for communication between the server (Raspberry Pi 4) and Pico W devices.

## Transport

| Channel | Port | Direction | Description |
|---------|------|-----------|-------------|
| UDP Unicast | 9090 | Bidirectional | Device ↔ Server commands |
| UDP Broadcast | 8765 | Server → Devices | Beat notifications |
| HTTPS | 8443 | Client → Server | Web client REST API |

All multi-byte fields are in **network byte order** (big-endian). All structs are packed (`__attribute__((__packed__))`).

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

Sent by a Pico W device to register with the server. Contains the device's unique board ID as a hex string.

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

Sent by a device to request the current tempo state. Uses the base message header only (optionally includes stale beat reference and period, but these are ignored by the server).

| Offset | Field | Type | Description |
|--------|-------|------|-------------|
| 0 | type | uint8_t | `3` |
| 1 | beat_time_ref | uint64_t | (unused, may be zero) |
| 9 | tempo_period_us | uint32_t | (unused, may be zero) |

**Size**: 13 bytes

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

Sent by the server to change the active LED program on a device.

| Offset | Field | Type | Description |
|--------|-------|------|-------------|
| 0 | type | uint8_t | `7` |
| 1 | program_id | uint16_t | New LED program ID |

**Size**: 3 bytes

---

### NEXT_BEAT (8)

Sent by the server (unicast) to each registered device before each beat. Devices use `next_beat_time_ref` to schedule LED updates at the precise moment.

| Offset | Field | Type | Description |
|--------|-------|------|-------------|
| 0 | type | uint8_t | `8` |
| 1 | next_beat_time_ref | uint64_t | Predicted time of next beat (microseconds, server clock) |
| 9 | tempo_period_us | uint32_t | Current beat period in microseconds |
| 13 | beat_count | uint32_t | Running beat counter |
| 17 | program_id | uint16_t | Active LED program ID |

**Size**: 19 bytes

---

### BEAT (9)

Broadcast to all devices when a beat is detected. Informational — devices primarily use NEXT_BEAT for timing.

| Offset | Field | Type | Description |
|--------|-------|------|-------------|
| 0 | type | uint8_t | `9` |
| 1 | beat_time_ref | uint64_t | Time of detected beat (microseconds, server clock) |
| 9 | tempo_period_us | uint32_t | Current beat period in microseconds |
| 13 | beat_count | uint32_t | Running beat counter |
| 17 | program_id | uint16_t | Active LED program ID |

**Size**: 19 bytes

---

## Synchronization Flow

1. **Registration**: Device sends HELLO_REQUEST with its board ID. Server responds with HELLO_RESPONSE containing an assigned client_id. The server tracks the device's address for future unicast messages.

2. **Time Sync**: Device performs multiple rounds of TIME_REQUEST/TIME_RESPONSE exchanges to establish a clock offset between device and server clocks. This offset allows the device to convert server timestamps to local time for precise LED scheduling.

3. **Tempo Sync**: Device sends TEMPO_REQUEST. Server responds with TEMPO_RESPONSE containing the current beat reference time, period, and program.

4. **Steady State**: Server sends NEXT_BEAT messages (unicast) before each beat and BEAT messages (broadcast) at each beat detection. If the tempo changes significantly, the device may re-enter tempo sync (TEMPO_SYNCED → TEMPO_SYNCED self-transition).
