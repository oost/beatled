---
title: HTTP API Reference
layout: default
nav_order: 5
---

# HTTP API Reference

REST API served over HTTPS on port **8443**. Used by the React, iOS, and macOS clients to monitor and control the server.

## Endpoints Summary

| Endpoint               | Method   | Purpose                                           |
| ---------------------- | -------- | ------------------------------------------------- |
| `/api/health`          | GET      | Lightweight health check (no auth required)       |
| `/api/status`          | GET      | Service status, tempo, client list                |
| `/api/service/control` | POST     | Start/stop services                               |
| `/api/tempo`           | GET      | Current tempo and time reference                  |
| `/api/program`         | GET/POST | Get/set LED program                               |
| `/api/log`             | GET      | Server log tail                                   |
| `/api/devices`         | GET      | Connected device list with IPs and last seen time |

All POST endpoints validate request body size (max 4 KB) and required JSON fields. When `--api-token` is set, all endpoints require `Authorization: Bearer <token>`.

--- All responses are JSON with `Content-Type: text/json; charset=utf-8`.

## Authentication

When the server is started with `--api-token`, every request must include a Bearer token:

```
Authorization: Bearer <token>
```

Unauthenticated requests receive a `401 Unauthorized` response:

```json
{ "error": "Unauthorized" }
```

## Rate Limiting

POST endpoints are rate-limited to **60 requests per 10-second** sliding window. Exceeding the limit returns `429 Too Many Requests`:

```json
{ "error": "Too many requests" }
```

## Request Limits

All POST endpoints reject request bodies larger than **4 KB** with a `400 Bad Request` response.

---

## Endpoints

### GET /api/health

Lightweight health check endpoint. Returns a fixed response with no authentication required. Used by clients to check server reachability.

**Response** `200 OK`

```json
{ "status": "ok" }
```

---

### GET /api/status

Returns the server health status, service states, current tempo, and connected device count.

**Response** `200 OK`

```json
{
  "message": "It's all good!",
  "status": {
    "beat_detector": true,
    "udp_server": false,
    "tempo_broadcaster": false
  },
  "tempo": 120.5,
  "deviceCount": 3
}
```

| Field | Type | Description |
|-------|------|-------------|
| `message` | string | Status message |
| `status` | object | Map of service ID to running state (boolean) |
| `tempo` | number | Current detected tempo in BPM |
| `deviceCount` | number | Number of connected Pico W devices |

---

### POST /api/service/control

Start or stop a server service.

**Request body**

```json
{
  "id": "beat_detector",
  "status": true
}
```

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `id` | string | Yes | Service identifier |
| `status` | boolean | Yes | `true` to start, `false` to stop |

**Response** `200 OK`

```json
{
  "status": true
}
```

**Error responses**

| Status | Condition |
|--------|-----------|
| `400 Bad Request` | Missing `id` or `status` field, body too large, or invalid JSON |
| `404 Not Found` | Service ID not recognized |
| `429 Too Many Requests` | Rate limit exceeded |

---

### GET /api/tempo

Returns the current tempo and beat time reference.

**Response** `200 OK`

```json
{
  "tempo": 128.0,
  "time_ref": 1707900000000000
}
```

| Field | Type | Description |
|-------|------|-------------|
| `tempo` | number | Current tempo in BPM |
| `time_ref` | number | Beat reference timestamp in microseconds since epoch |

---

### GET /api/program

Returns the active LED program and the list of available programs.

**Response** `200 OK`

```json
{
  "message": "Current program is 2",
  "programId": 2,
  "programs": [
    { "name": "Snakes!", "id": 0 },
    { "name": "Random data", "id": 1 },
    { "name": "Sparkles", "id": 2 },
    { "name": "Greys", "id": 3 },
    { "name": "Drops", "id": 4 },
    { "name": "Solid!", "id": 5 },
    { "name": "Fade", "id": 6 },
    { "name": "Fade Color", "id": 7 }
  ]
}
```

| Field | Type | Description |
|-------|------|-------------|
| `message` | string | Human-readable status |
| `programId` | number | Active program ID |
| `programs` | array | Available LED programs with `name` and `id` |

---

### POST /api/program

Set the active LED program. The program change is broadcast to all connected Pico W devices.

**Request body**

```json
{
  "programId": 3
}
```

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `programId` | number | Yes | Program ID (0-7) |

**Response** `200 OK`

```json
{
  "message": "Updated program to 3"
}
```

**Error responses**

| Status | Condition |
|--------|-----------|
| `400 Bad Request` | Missing `programId` field, body too large, or invalid JSON |
| `429 Too Many Requests` | Rate limit exceeded |

---

### GET /api/log

Returns the server log tail as a JSON array.

**Response** `200 OK`

```json
[
  "2026-02-14 12:00:01 [info] Beat detected at 128.0 BPM",
  "2026-02-14 12:00:02 [info] Client registered: 192.168.1.42"
]
```

---

### GET /api/devices

Returns the list of connected Pico W devices.

**Response** `200 OK`

```json
{
  "devices": [
    {
      "client_id": 1,
      "board_id": "E6614103E72B6A2F",
      "ip_address": "192.168.1.42",
      "last_status_time": 1707900120000000,
      "port_name": "pico-freertos",
      "git_sha": "1a2b3c4-dirty",
      "build_time_us": 1707800000000000,
      "owd_us": 1500,
      "qos": {
        "current_offset_us": -42,
        "uptime_us": 9999,
        "median_rtt_us": 1234,
        "next_beat_gap_total": 3,
        "intercore_drop_total": 0,
        "time_sync_outlier_total": 5,
        "valid_sample_count": 8,
        "last_applied_program_seq": 7,
        "server_received_at_us": 1707900120000000,
        "last_rtt_us": 555
      }
    }
  ],
  "count": 1
}
```

| Field | Type | Description |
|-------|------|-------------|
| `devices` | array | Connected device objects |
| `devices[].client_id` | number | Server-assigned client ID |
| `devices[].board_id` | string | Pico W unique board identifier (hex) |
| `devices[].ip_address` | string | Device IP address |
| `devices[].last_status_time` | number | Wall-clock timestamp (microseconds) of the most recent HELLO or TEMPO_REQUEST received from this device |
| `devices[].port_name` | string | Firmware port: `pico`, `pico-freertos`, `posix`, `posix-freertos`, `esp32`, or `unknown`. Empty for v2-or-older firmware. |
| `devices[].git_sha` | string | Short Git SHA of the firmware build (possibly with `-dirty`). Empty for pre-v3 clients. |
| `devices[].build_time_us` | number | Unix epoch microseconds of the firmware build. 0 for pre-v3 clients. |
| `devices[].owd_us` | number | Server-smoothed one-way delay estimate (EWMA over the controller's reported `owd_us_estimate`). |
| `devices[].qos` | object \| null | Protocol v4 diagnostic snapshot. `null` until the device has sent its first TEMPO_REQUEST or STATUS_RESPONSE. Fields: `current_offset_us`, `uptime_us`, `median_rtt_us`, `next_beat_gap_total`, `intercore_drop_total`, `time_sync_outlier_total`, `valid_sample_count`, `last_applied_program_seq`, `server_received_at_us`, `last_rtt_us`. |
| `count` | number | Total connected devices |

---

## GET /api/qos

Fleet-wide aggregates over the controllers' v4 QoS snapshots. Useful
for the React Fleet QoS card and any external observability tooling.

```json
{
  "device_count": 2,
  "reporting_count": 2,
  "min_offset_us": -42,
  "max_offset_us": 17,
  "fleet_skew_us": 59,
  "mean_rtt_us": 1234,
  "min_rtt_us": 1100,
  "max_rtt_us": 1400,
  "slowest_device_board_id": "E6614103E72B6A2F",
  "total_next_beat_gap": 3,
  "total_intercore_drops": 0,
  "total_time_sync_outliers": 5,
  "thresholds": { "skew_warn_us": 5000, "skew_fail_us": 20000 },
  "health": "ok"
}
```

| Field | Type | Description |
|-------|------|-------------|
| `device_count` | number | Total registered controllers |
| `reporting_count` | number | Number of controllers that have sent at least one v4 QoS snapshot |
| `min_offset_us` / `max_offset_us` | number \| null | Lowest / highest reported controller-side server-time-offset |
| `fleet_skew_us` | number \| null | `max_offset_us - min_offset_us`; the spread that drives the health pip |
| `min_rtt_us` / `mean_rtt_us` / `max_rtt_us` | number \| null | Aggregate over each controller's `median_rtt_us` |
| `slowest_device_board_id` | string | board_id of the device whose `median_rtt_us` is the largest |
| `total_next_beat_gap` | number | Sum of `next_beat_gap_total` across all reporting devices |
| `total_intercore_drops` | number | Sum of `intercore_drop_total` |
| `total_time_sync_outliers` | number | Sum of `time_sync_outlier_total` |
| `thresholds` | object | Current values of `--qos-skew-warn-us` and `--qos-skew-fail-us` |
| `health` | string | `"ok"`, `"warn"`, `"fail"`, or `"unknown"` (zero reporting devices). Computed server-side from `fleet_skew_us` vs. `thresholds` plus the drop / outlier totals — any non-zero forces `fail`. |

---

## CORS

When the server is started with `--cors-origin`, all API responses include:

```
Access-Control-Allow-Origin: <origin>
Access-Control-Allow-Headers: Origin, X-Requested-With, Content-Type, Accept, Authorization
```

`OPTIONS` and `HEAD` requests to any `/api/*` path return `200 OK` for preflight support.

---

## Error Format

All error responses use a consistent JSON format:

```json
{
  "error": "Description of the error"
}
```
