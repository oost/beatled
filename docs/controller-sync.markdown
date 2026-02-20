---
title: Controller Registration and Synchronization
layout: default
parent: Server ↔ Controller Communication
nav_order: 2
---

# Controller Registration and Synchronization

Controllers follow a four-phase startup sequence before LEDs become active. Each phase corresponds to a state transition in the [device state machine](../controller.html#device-state-machine).

```mermaid
sequenceDiagram
    participant C as Controller
    participant S as Server

    Note over C: STATE: INITIALIZED
    C->>S: HELLO_REQUEST (board_id)
    S->>C: HELLO_RESPONSE (client_id)
    Note over C: STATE: REGISTERED

    loop Time Sync
        C->>S: TIME_REQUEST (orig_time)
        S->>C: TIME_RESPONSE (orig, recv, xmit)
        Note over C: Calculate clock offset
    end
    Note over C: STATE: TIME_SYNCED

    C->>S: TEMPO_REQUEST
    S->>C: TEMPO_RESPONSE (beat_ref, period, program)
    Note over C: STATE: TEMPO_SYNCED

    loop Steady State
        S->>C: NEXT_BEAT (beat_ref, period, count, program)
        Note over C: Schedule LED update at beat_ref
        S-->>C: BEAT (broadcast, informational)
    end
```

## Phases

### 1. Registration

The controller sends a [HELLO_REQUEST](protocol.html#hello_request-1) containing its unique board ID. The server assigns a `client_id` and registers the device's IP address for future unicast messages, then replies with a [HELLO_RESPONSE](protocol.html#hello_response-2).

### 2. Time Sync

The controller performs multiple rounds of [TIME_REQUEST](protocol.html#time_request-5) / [TIME_RESPONSE](protocol.html#time_response-6) exchanges using the NTP symmetric algorithm to establish a clock offset between device and server clocks:

```
offset = ((T2 - T1) + (T3 - T4)) / 2
```

This offset is added to server timestamps so the controller can schedule LED updates at the precise moment a beat will occur.

### 3. Tempo Sync

The controller sends a [TEMPO_REQUEST](protocol.html#tempo_request-3). The server replies with a [TEMPO_RESPONSE](protocol.html#tempo_response-4) containing the current beat reference time, beat period in microseconds, and active LED program ID.

### 4. Steady State

The server sends [NEXT_BEAT](protocol.html#next_beat-8) unicast messages before each beat so the controller can pre-schedule its LED update. It also broadcasts [BEAT](protocol.html#beat-9) messages at each detected beat for informational use.

If the tempo changes significantly, the controller re-enters tempo sync (TEMPO_SYNCED → TEMPO_SYNCED self-transition in the state machine).
