# mDNS Service Discovery & TLS — Architectural Notes
*2026-02-20*

## Problem
Controllers (ESP32 / Pico W) need to discover the server's IP address on the local WiFi network without manual configuration.

## Recommendation: mDNS + DNS-SD

mDNS is the right tool — zero-configuration, no DNS server required, well-supported on all target platforms.

### How it works
- The server advertises itself via mDNS, e.g. as `beatled.local` or with a DNS-SD service type `_beatled._tcp.local`
- Controllers query mDNS on the local subnet to resolve the IP — no hardcoded addresses needed
- DNS-SD lets controllers query by *service type* rather than hostname, so they don't need to know `beatled.local` in advance

### Platform support

| Platform | Library / Mechanism |
|---|---|
| ESP32 | `ESPmDNS` (Arduino) or `mdns` component (ESP-IDF) |
| Pico W | lwIP mDNS (`MDNS_ENABLED` in `lwipopts.h`) |
| macOS server | Native `dns-sd` CLI, or Node.js `mdns` / `bonjour` packages |
| Linux server | `avahi-daemon` |

---

## TLS: mkcert with `.local` domains

`mkcert` can generate certificates for `.local` hostnames:

```bash
mkcert beatled.local
```

This creates a cert signed by the local mkcert CA, valid for `beatled.local`.

### Where it's trusted automatically
- macOS / iOS / Windows machines where `mkcert -install` has been run

### Where extra steps are needed
- **iOS client**: Install the mkcert CA profile (`rootCA.pem`) via Settings → General → VPN & Device Management
- **ESP32 / Pico controllers**: Embed `~/.local/share/mkcert/rootCA.pem` in firmware and pass it to the TLS stack (mbedTLS on ESP-IDF or equivalent)

### Browser caveat
Browsers handle `.local` mDNS resolution inconsistently — Chrome/Safari on macOS work, but test early on target platforms.

---

## Practical Architecture

| Path | Protocol | TLS |
|---|---|---|
| Controller ↔ Server | WebSocket (`ws://`) | No — local network, embedded TLS adds complexity |
| iOS/Web client ↔ Server | WebSocket (`wss://`) | Yes — mkcert cert for `beatled.local` |

**Rationale:** Keeping TLS off the controller↔server path avoids embedding CA certs in firmware, while still securing the user-facing client connections.

---

## Implementation Checklist (future)
- [ ] Server: advertise `_beatled._tcp` via mDNS on startup
- [ ] ESP32: query `_beatled._tcp` on connect, fall back to `beatled.local`
- [ ] Pico W: same, using lwIP mDNS
- [ ] Server: generate / use mkcert cert for `beatled.local` for HTTPS/WSS
- [ ] iOS: install mkcert CA profile in dev setup docs
