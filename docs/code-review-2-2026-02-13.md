# Beatled Code Review #2 — 2026-02-13

## Summary

| Severity | Server | Pico | Client | Infra | Total |
|----------|--------|------|--------|-------|-------|
| CRITICAL | 2 | 4 | 0 | 2 | **8** |
| HIGH | 3 | 5 | 2 | 3 | **13** |
| MEDIUM | 8 | 8 | 10 | 14 | **40** |
| LOW | 3 | 2 | 4 | 3 | **12** |
| **Total** | **16** | **19** | **16** | **22** | **73** |

---

## CRITICAL (8)

### C1. Race condition: Registry access without synchronization (Pico)
**Files**: `src/ws2812/ws2812.c:106-149`, `src/command/tempo/tempo.c:57-61`

`led_update()` runs on core1 in a tight loop and reads `_tempo_period_us`, `_next_beat_time` (uint64_t) without any lock. Core0 writes these via `update_tempo()`. On RP2040, 64-bit reads are non-atomic — this causes torn reads, incorrect LED timing, and sync loss.

**Fix**: Lock around the reads in `led_update()` or copy to locals under lock in `update_tempo()`.

### C2. Integer overflow in scale8() (Pico)
**File**: `src/ws2812/ws2812.c:41-59`

`scale8(value, range)` divides via iterative bit-shift but has no guard for `range == 0` (division-by-zero equivalent) or `value > range` (incorrect output).

**Fix**: Add `if (range == 0) return 0; if (value >= range) return 255;`

### C3. Memory ownership ambiguity in UDP callback (Pico)
**File**: `src/hal/network/ports/pico/udp_server.c:15-60`

`dgram_recv()` mallocs `server_msg`, passes it to `process_response_()`. If enqueue succeeds, `handle_event()` frees it later. If enqueue fails, the callback frees it. But there's no contract — if the callback itself returns error after a successful internal enqueue, double-free occurs. Also, malloc in lwIP callback context is unsafe.

**Fix**: Document ownership contract. Consider pre-allocated message pool.

### C4. Unsafe memcpy on generic template type (Server)
**File**: `server/src/server/udp/include/udp/udp_buffer.hpp:49`

```cpp
template <class T> void set_data(const T &data) {
    memcpy(&data_, &data, sizeof(T));  // UB if T is non-trivially-copyable
}
```

**Fix**: Add `static_assert(std::is_trivially_copyable_v<T>)`.

### C5. Emergency buffer allocation race (Server)
**File**: `server/src/beat_detector/audio/audio_buffer_pool.hpp:98-103`

When pool is exhausted, emergency allocation increments `total_pool_size_` and `buffer_count_` outside the mutex, creating a race condition.

**Fix**: Perform allocation and counter updates inside the locked section.

### C6. Docker base images not pinned to digest (Infra)
**Files**: `docker/Dockerfile.beatled:26,36`, `docker/Dockerfile.beatled-dockcross:27,34`

`FROM node` and `FROM debian:bookworm-slim` use mutable tags. A compromised or changed upstream image breaks builds or introduces supply chain risk.

**Fix**: Pin to digest, e.g. `FROM node@sha256:<digest>`.

### C7. Client .env has typo and is version-controlled (Infra)
**File**: `client/.env`

```
PUBLIC_URL=http://locahost:5173
```
Typo (`locahost`), and `.env` is committed to git.

**Fix**: Fix typo, add `.env` to `.gitignore`, use `.env.example` instead.

### C8. State machine handles invalid transitions silently (Pico)
**File**: `src/command/command.c` — `handle_state_change()`

```c
int handle_state_change(void *event_data) {
  state_event_t *state_event = (state_event_t *)event_data;
  state_manager_set_state(state_event->next_state);  // May fail
  return 0;  // Always returns 0!
}
```

**Fix**: Propagate the return value of `state_manager_set_state()`.

---

## HIGH (13)

### H1. Raw `new` with shared_ptr (Server)
**Files**: `server/src/server/server.cpp:61-62`, `server/src/application.cpp:117-118`

`std::shared_ptr<asio::thread> thread(new asio::thread(...))` — use `std::make_shared` instead for exception safety.

### H2. Division by zero in StateManager (Server)
**File**: `server/src/core/state_manager.cpp:20`

```cpp
.tempo_period_us = static_cast<uint32_t>(60 * 1000000UL / tempo_),
```
If `tempo_` is 0 (before first beat), this is UB.

**Fix**: Guard with `tempo_ > 0.0f ? ... : 0`.

### H3. Missing JSON schema validation (Server)
**File**: `server/src/server/http/api_handler.cpp:65-67`

`json::parse(req->body())` — no size limit, no field validation. OOM/crash possible with large payloads.

**Fix**: Validate fields exist, add body size limit.

### H4. Unsafe ISR alarm access (Pico)
**File**: `src/hal/ws2812/ports/pico/ws2812_dma.c:29-40`

DMA ISR reads/writes `reset_delay_alarm_id` without spinlock. Volatile alone is insufficient for ISR/main synchronization on RP2040.

**Fix**: Use `spin_lock_blocking()` / `spin_unlock()`.

### H5. Event queue silently drops messages when full (Pico)
**File**: `src/event/event_queue.c:23-35`

Queue has fixed size 20. When full, `hal_queue_add_message` returns false but callers often don't handle it — tempo updates, state transitions silently lost.

**Fix**: Increase queue size, add error logging/backpressure, or use blocking queue for critical events.

### H6. CORS validation missing in client (Client)
**File**: `client/src/lib/api.ts:1-6`

`setAPIHost()` accepts any URL with no validation. An attacker could redirect API calls.

**Fix**: Whitelist allowed hosts.

### H7. XSS risk in log display (Client)
**File**: `client/src/views/log.tsx:69-73`

Server logs rendered directly. React auto-escapes text content, so this is mitigated, but the pattern is fragile — any switch to `dangerouslySetInnerHTML` would be exploitable.

**Fix**: Add explicit sanitization or ensure never using innerHTML.

### H8. Certificate script uses 2048-bit RSA (Infra)
**File**: `scripts/create-certs.sh:29,37`

Mixes 2048-bit and 4096-bit keys. Parameter validation occurs after use. Paths unquoted.

**Fix**: Standardize to 4096-bit, validate params first, quote all paths.

### H9. No build artifact uploads in CI (Infra)
**Files**: `.github/workflows/*.yml`

No artifacts saved. Failed builds require full local reproduction to debug.

**Fix**: Add `actions/upload-artifact` for build logs on failure.

### H10. Missing test coverage gating in CI (Infra)
**Files**: `.github/workflows/server-ci.yml`, `client-ci.yml`

Tests run but may not block merges if branch protection isn't configured.

**Fix**: Verify required status checks, add test result reporting.

### H11. Negative modulo in LED pattern dispatch (Pico)
**File**: `src/ws2812/ws2812_patterns.c:62-66`

`pattern_idx % num_patterns` — if `pattern_idx < 0`, C modulo can return negative result (implementation-defined).

**Fix**: `((pattern_idx % n) + n) % n`.

### H12. Message validation uses `>=` instead of `>` (Pico)
**File**: `src/command/command.c:52`

```c
if (!event_data || sizeof(beatled_message_t) >= data_length) {
```
This rejects messages of exactly `sizeof(beatled_message_t)`, which should be valid.

**Fix**: Change `>=` to `>`.

### H13. Server.markdown heading says "Client" (Infra)
**File**: `docs/server.markdown:7`

Front-matter says "Beatled Server" but H1 heading says "Beatled Client".

**Fix**: Change heading to `# Beatled Server`.

---

## MEDIUM (40)

### Server (8)
| # | Issue | File | Line |
|---|-------|------|------|
| M1 | `is_running()` not const | `core/interfaces/service_controller.hpp` | 28 |
| M2 | Redundant `std::move` on return (defeats NRVO) | `audio/audio_buffer_pool.hpp` | 116 |
| M3 | Missing TLS options (`no_compression`, `prefer_server_ciphers`) | `http/http_server.cpp` | 121-127 |
| M4 | No request body size limit | `http/api_handler.cpp` | — |
| M5 | Tight lambda coupling in Application callbacks | `application.cpp` | 33-52 |
| M6 | Server/Application thread pool code duplication | `server.cpp` / `application.cpp` | — |
| M7 | Circular dependency: core → server | `src/CMakeLists.txt` | 9-12 |
| M8 | Missing compiler warning flags (`-Wall -Wextra`) | `server/CMakeLists.txt` | — |

### Pico (8)
| # | Issue | File | Line |
|---|-------|------|------|
| M9 | Unused ISR thread (dead code) | `src/process/isr_thread.c` | 1-6 |
| M10 | Bit-shift message types not named constants | `command/tempo/tempo.c` | 70-71 |
| M11 | `printf()` in ISR context | `hal/ws2812/ports/pico/ws2812_dma.c` | 14 |
| M12 | Magic timing constants undocumented | Multiple state files | — |
| M13 | State machine has no timeout/recovery | `state_manager.c` | — |
| M14 | Command pipeline lacks centralized state validation | `command/command.c` | — |
| M15 | LED module tightly coupled to registry | `ws2812/ws2812.c` | — |
| M16 | HAL queue mixes stack/heap allocation across ports | `hal/queue/ports/` | — |

### Client (10)
| # | Issue | File | Line |
|---|-------|------|------|
| M17 | Chart config recreated every render (no useMemo) | `components/BeatChart.tsx` | 56-67 |
| M18 | Console interception unbounded, no level distinction | `lib/console.ts` | 14-18 |
| M19 | Global mutable state (`API_HOST`, `tempoHistory`) | `lib/api.ts:1`, `views/status.tsx:23` |
| M20 | API layer: no error classification, no retry, no timeout | `lib/api.ts` | — |
| M21 | Loader modifies global state (not pure) | `views/status.tsx:26-38` | — |
| M22 | Missing Content Security Policy headers | `index.html` | — |
| M23 | No input validation on token field | `views/config.tsx:40-44` | — |
| M24 | Generic error handling loses error details | `lib/status.ts`, `lib/program.ts` | — |
| M25 | Accessibility: error messages not marked as `role="alert"` | `views/status.tsx:90` | — |
| M26 | PWA devOptions enabled (caching issues) | `vite.config.ts:27-29` | — |

### Infra (14)
| # | Issue | File | Line |
|---|-------|------|------|
| M27 | Docker: No explicit user UID/GID | `Dockerfile.beatled:40` | — |
| M28 | Docker: APT cache not cleaned in build-client | `Dockerfile.beatled:28-29` | — |
| M29 | Docker: scripts/ copied wholesale | `Dockerfile.beatled:44` | — |
| M30 | Docker: Build base image inconsistency (trixie vs bookworm) | `Dockerfile.builder`, `Dockerfile.dockcross` | — |
| M31 | .dockerignore missing `.github/`, `test-docker/`, `.env*` | `.dockerignore` | — |
| M32 | CI: vcpkg dependencies not cached | `server-ci.yml:33-36` | — |
| M33 | CI: No npm audit step | `client-ci.yml` | — |
| M34 | CI: Pages branch hardcoded to feature branch | `docs/_config.yml:136` | — |
| M35 | Docs: Incomplete setup instructions (vcpkg, submodules) | `README.md` | — |
| M36 | Scripts: Inconsistent error handling (`set -x` vs not) | `utils/*.sh` | — |
| M37 | Scripts: Portability issues in beatled.sh | `utils/beatled.sh` | — |
| M38 | Scripts: Service install no validation | `scripts/install-service.sh` | — |
| M39 | Scripts: Deployment script no tar validation | `utils/copy-tar-files.sh` | — |
| M40 | vcpkg.json: No version constraints on dependencies | `server/vcpkg.json` | — |

---

## LOW (12)

### Server (3)
| # | Issue | File |
|---|-------|------|
| L1 | String param copies in APIHandler constructor | `http/api_handler.hpp:23` |
| L2 | Inconsistent namespace organization | Multiple |
| L3 | Outdated clang-tidy/cppcheck comments | `server/CMakeLists.txt:3-6` |

### Pico (2)
| # | Issue | File |
|---|-------|------|
| L4 | Busy-wait DNS resolution (30s blocking) | `hal/network/ports/pico/dns.c:43-57` |
| L5 | printf() overhead in LED loop (every 1000 cycles) | `ws2812/ws2812.c:140-145` |

### Client (4)
| # | Issue | File |
|---|-------|------|
| L6 | No lazy loading for routes | `routes/index.tsx` |
| L7 | ESLint: deprecated prop-types rule, no import ordering | `.eslintrc.json` |
| L8 | Missing `resolveJsonModule` in tsconfig | `tsconfig.json` |
| L9 | No bundle size analysis configured | `vite.config.ts` |

### Infra (3)
| # | Issue | File |
|---|-------|------|
| L10 | Conditional workflow triggers for docs-only changes | `.github/workflows/ci.yml:4-6` |
| L11 | npm caret ranges (^) vs tilde (~) | `client/package.json` |
| L12 | Ruby bundler version old (2.3.26) | `docs/Gemfile.lock:160` |

---

## Test Coverage Gaps

### Server — Missing Tests
- UDP malformed packet handling
- TLS handshake / certificate validation
- HTTP server integration tests
- Concurrent StateManager access
- BeatDetector exception propagation

### Pico — Missing Tests
- Inter-core synchronization (core0/core1 races)
- Queue full / message loss scenarios
- UDP timeout / malformed packets
- State machine timeout/recovery
- Malloc failure paths
- LED beat fraction edge cases (scale8)
- WS2812 DMA handling
- Pattern rendering

### Client — Missing Tests
- View components (status, program, log, config)
- useInterval hook
- Router integration (loaders, actions)
- Error scenarios (401, 500, timeout)
- BottomNav, RootContainer components

---

## Recommended Priority Order

### Phase 1: Critical Safety (Now)
1. **C1** — Registry race condition (pico core0/core1)
2. **C2** — scale8() guard for zero
3. **C4** — static_assert on memcpy template
4. **C5** — Audio pool emergency allocation race
5. **H2** — Division by zero in server StateManager
6. **H12** — Fix `>=` to `>` in validate_server_message
7. **C8** — Propagate state transition errors

### Phase 2: Security Hardening
8. **C6** — Pin Docker images to digest
9. **C7** — Fix .env typo, remove from git
10. **H3** — JSON input validation
11. **H6** — API host whitelist
12. **H8** — Certificate script 4096-bit keys

### Phase 3: Robustness
13. **H4** — ISR spinlock for alarm IDs
14. **H5** — Event queue overflow handling
15. **C3** — UDP memory ownership contract
16. **H1** — Replace raw `new` with make_shared
17. **H11** — Fix negative modulo in pattern dispatch
18. **M13** — State machine timeouts

### Phase 4: Quality & Tests
19. **M8** — Compiler warning flags
20. Server: Add UDP/TLS/HTTP integration tests
21. Pico: Add inter-core synchronization tests
22. Client: Add view component tests
23. **H9/H10** — CI artifact uploads + test gating

### Phase 5: Polish
24. Medium/Low items from all categories
