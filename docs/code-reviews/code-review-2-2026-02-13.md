# Beatled Code Review #2 — 2026-02-13

## Summary

| Severity | Server | Pico | Client | Infra | Total |
|----------|--------|------|--------|-------|-------|
| CRITICAL | 0 | 0 | 0 | 1 | **1** |
| HIGH | 1 | 1 | 2 | 3 | **7** |
| MEDIUM | 7 | 8 | 10 | 14 | **39** |
| LOW | 3 | 1 | 4 | 3 | **11** |
| **Total** | **11** | **10** | **16** | **21** | **58** |

---

## CRITICAL (1)

### C6. Docker base images not pinned to digest (Infra)
**Files**: `docker/Dockerfile.beatled:26,36`, `docker/Dockerfile.beatled-dockcross:27,34`

`FROM node` and `FROM debian:bookworm-slim` use mutable tags. A compromised or changed upstream image breaks builds or introduces supply chain risk.

**Fix**: Pin to digest, e.g. `FROM node@sha256:<digest>`.

---

## HIGH (7)

### H3. Missing JSON schema validation (Server)
**File**: `server/src/server/http/api_handler.cpp:65-67`

`json::parse(req->body())` — no size limit, no field validation. OOM/crash possible with large payloads.

**Fix**: Validate fields exist, add body size limit.

### H4. Unsafe ISR alarm access (Pico)
**File**: `src/hal/ws2812/ports/pico/ws2812_dma.c:29-40`

DMA ISR reads/writes `reset_delay_alarm_id` without spinlock. Volatile alone is insufficient for ISR/main synchronization on RP2040.

**Fix**: Use `spin_lock_blocking()` / `spin_unlock()`.

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

---

## MEDIUM (39)

### Server (7)
| # | Issue | File | Line |
|---|-------|------|------|
| M1 | `is_running()` not const | `core/interfaces/service_controller.hpp` | 28 |
| M2 | Redundant `std::move` on return (defeats NRVO) | `audio/audio_buffer_pool.hpp` | 116 |
| M3 | Missing TLS options (`no_compression`, `prefer_server_ciphers`) | `http/http_server.cpp` | 121-127 |
| M4 | No request body size limit | `http/api_handler.cpp` | — |
| M5 | Tight lambda coupling in Application callbacks | `application.cpp` | 33-52 |
| M6 | Server/Application thread pool code duplication | `server.cpp` / `application.cpp` | — |
| M7 | Circular dependency: core → server | `src/CMakeLists.txt` | 9-12 |

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

## LOW (11)

### Server (3)
| # | Issue | File |
|---|-------|------|
| L1 | String param copies in APIHandler constructor | `http/api_handler.hpp:23` |
| L2 | Inconsistent namespace organization | Multiple |
| L3 | Outdated clang-tidy/cppcheck comments | `server/CMakeLists.txt:3-6` |

### Pico (1)
| # | Issue | File |
|---|-------|------|
| L4 | Busy-wait DNS resolution (30s blocking) | `hal/network/ports/pico/dns.c:43-57` |

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

## Fixed Issues (removed from this review)

The following issues were resolved:

- ~~C1 Race condition: Registry access without synchronization~~ — snapshot-under-lock pattern in led_update()
- ~~C2 Integer overflow in scale8()~~ — guards for range==0 and value>=range
- ~~C3 Memory ownership ambiguity in UDP callback~~ — explicit free on enqueue failure
- ~~C4 Unsafe memcpy on generic template type~~ — static_assert(is_trivially_copyable_v) added
- ~~C5 Emergency buffer allocation race~~ — allocation moved inside mutex
- ~~C8 State machine handles invalid transitions silently~~ — return value propagated
- ~~H1 Raw new with shared_ptr~~ — replaced with make_shared
- ~~H2 Division by zero in StateManager~~ — tempo_ > 0 guard added
- ~~H5 Event queue silently drops messages~~ — queue size increased to 64, error logging added
- ~~H11 Negative modulo in LED pattern dispatch~~ — ((x%n)+n)%n pattern applied
- ~~H12 Message validation uses >= instead of >~~ — corrected to >
- ~~H13 Server.markdown heading says "Client"~~ — corrected to "Beatled Server"
- ~~M8 Missing compiler warning flags~~ — -Wall -Wextra -Wpedantic added
- ~~L5 printf() overhead in LED loop~~ — gated behind BEATLED_VERBOSE_LOG
- ~~C7 Client .env has typo and is version-controlled~~ — typo fixed, .env added to .gitignore, .env.example created
