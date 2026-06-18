# OTA-update feasibility for the FreeRTOS controllers — 2026-06-16

## Context

Assessment of whether the FreeRTOS controller could support OTA (over-the-air)
firmware updates. There are two FreeRTOS targets and the answer differs sharply
between them, so this covers both — what's required, effort, risks, and a
recommended delivery mechanism. No implementation is proposed here. The
motivation is concrete: the protocol-v5 bump forced a manual fleet reflash,
which is exactly the pain OTA removes.

## Current state (both targets)

- **No OTA anywhere.** No `esp_ota` / `picowota` / `flash_range_program` calls
  in `controller/`. Today firmware is flashed physically: Pico W via UF2 to the
  BOOTSEL drive; ESP32 via `idf.py flash` over serial.
- **Version is already tracked end-to-end.** `controller/cmake/gen_version.cmake`
  stamps git SHA + build time into `beatled_version.h`, and HELLO_REQUEST carries
  `git_sha[16]` + `build_time_us` + `port_name[16]`
  (`controller/lib/beatled_protocol/include/beatled/protocol.h`). The server
  already knows every device's running build — a natural basis for "is an update
  needed?".
- **Networking present:** lwIP + Wi-Fi, but the control plane is connectionless
  UDP (raw lwIP API). The beat server already runs an HTTP server (restinio) and
  serves static files — it could host firmware artifacts.

## Target A — `esp32-freertos` (ESP32-C3, 4 MB flash): well-supported

ESP-IDF has first-class OTA, so this is the tractable path.

- **What's needed:** switch the partition table from single-app
  (`controller/esp32/sdkconfig.defaults`: `CONFIG_PARTITION_TABLE_SINGLE_APP=y`)
  to a two-OTA layout (`ota_0`/`ota_1` + `otadata`); integrate
  `esp_https_ota` (download → write inactive slot → set boot partition →
  reboot); enable rollback + image validation; optionally secure boot / signed
  images.
- **Effort:** low–moderate — mostly config + a self-contained OTA task. The
  threaded lwIP (`NO_SYS=0`) ESP-IDF already uses makes the HTTP client trivial.
- **Risks:** getting rollback/validation right so a bad image self-recovers.
  Flash is not a concern: the C3 module is 4 MB (now pinned in
  `sdkconfig.defaults`; the build previously inherited IDF's 2 MB default),
  which leaves room for two ~1.9 MB app slots + `otadata` + NVS. RAM is
  ample too — the C3's 400 KB SRAM (no PSRAM) easily covers
  `esp_https_ota`'s TLS/HTTP buffers.
- **Note:** the ESP32 port itself is still a *proposed* port
  (`docs/code-reviews/esp32-port-analysis-2026-02-16.md`), so OTA here depends
  on that port landing first.

## Target B — `pico-freertos` (Pico W / RP2040): possible but heavy

The RP2040 has **no native OTA**: it XIP-boots a flat image from a single flash
region via the ROM bootloader; there's no A/B partition concept.

- **What's needed:**
  1. A custom second-stage bootloader + flash partition scheme (the established
     open-source route is **picowota**): reserve a download/staging region and an
     active-image region in the 2 MB flash; bootloader validates + boots the
     active image and can swap in a freshly-downloaded one.
  2. A **reliable** download channel. The current transport is UDP (lossy, no
     ordering) — unsuitable for a firmware blob. Add a TCP/HTTP client over lwIP
     for the transfer (the FreeRTOS `NO_SYS=0` migration in
     `docs/code-reviews/freertos-migration-2026-02-16.md` makes a sockets/HTTP
     client much easier than the bare-metal `NO_SYS=1` callback model).
  3. Self-flash via `flash_range_program` — **must execute from RAM with IRQs
     disabled and the other core not XIP-executing from flash.** This is the
     delicate part: Core 1 runs the WS2812 LED loop from flash, so it must be
     parked/RAM-resident during the write or the device faults.
  4. Image integrity (CRC/hash, ideally signature) + power-loss-safe swap so a
     mid-update reset can't brick the unit.
- **Effort:** a real project (bootloader + partitioning + transport + flash
  + validation), not a config change.
- **Constraints:** 2 MB flash / 264 KB RAM. The LED firmware is small, so
  dual ~1 MB images fit, but it's tight and the Core-1 coexistence is the key
  hazard.

## Recommended delivery mechanism (applies to either target)

A **pull model anchored on the existing version data**:

1. Server publishes the latest firmware per `port_name` (build + `git_sha` /
   `build_time_us`) and serves the artifact over its existing HTTP server.
2. Server advertises "latest available version" — cleanest is a small addition
   to the control plane (a field the controller already receives, or a tiny new
   message); since HELLO already reports each device's running build, the server
   can tell who's stale.
3. Controller compares, then **pulls the image over HTTP/TCP** (reliable),
   verifies a hash/signature, writes the inactive slot, and reboots into it.
   Bulk transfer stays off the UDP beat path so it can't disturb timing.
4. Guardrails: signed/verified images, automatic rollback on boot-failure, and
   OTA gated to not run mid-set. Mind protocol-major compatibility — an OTA that
   crosses a protocol-major bump (e.g. v5→v6) must still hand off cleanly.

This reuses the HTTP server and the existing version fields rather than
inventing a parallel channel.

## Bottom line

- **ESP32-C3 (esp32-freertos):** yes — native ESP-IDF OTA; the pragmatic target
  if OTA matters. The 4 MB flash and 400 KB SRAM are comfortable for a two-OTA
  layout. Gated on the ESP32 port landing.
- **Pico W (pico-freertos):** yes in principle, but it's a substantial effort
  (custom bootloader + partitioning + reliable transport + RAM-resident
  self-flash beside the LED core). Justified mainly if Pico W stays the primary
  deployed hardware and manual reflashing is genuinely painful.

## Suggested next step

If pursued, the lowest-risk validation is an **ESP32 spike**: two-OTA partition
table + `esp_https_ota` pulling a test build from the beat server, with rollback
enabled — provable in isolation before wiring it into the beatled
version/trigger flow. A Pico W path would start with a standalone `picowota`
bootloader prototype.
