---
title: HTTP API Reference
layout: default
nav_order: 5
---

# HTTP API Reference

REST API served over HTTPS on port **8443**. All responses are JSON with `Content-Type: text/json; charset=utf-8`.

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
      "last_seen": 1707900120000000
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
| `devices[].last_seen` | number | Last communication timestamp (microseconds) |
| `count` | number | Total connected devices |

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
