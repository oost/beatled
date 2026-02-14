# Code Review - 2026-02-13

## CRITICAL

| # | Component | Issue | Location |
|---|-----------|-------|----------|
| 1 | Server | Unsafe `reinterpret_cast` without bounds check on UDP messages | udp_request_handler.cpp:56,78 |
| 2 | Pico | Use-after-free: `event_data` freed then accessed in dispatch | command.c:56 |
| 3 | Pico | `exit()` calls on non-fatal errors (queue overflow crashes device) | hello.c:50, time.c:74, next_beat.c:50 |
| 4 | Pico | Race conditions on LED globals (`ws2812.c`) - no mutex between `update_tempo()` and `led_update()` | ws2812.c:41-150 |
| 5 | Pico | Missing bounds validation in protocol parsing (division by zero, buffer overflow) | command.c:52-91 |
| 6 | Pico | UDP socket never closed - no cleanup function | udp_server.c:93 |
| 7 | Infra | vcpkg unpinned in Docker (`git checkout master`) vs pinned in CI | Dockerfile.builder:31 |
| 8 | Infra | Pages workflow triggers on `feature/build-steps` not `master` | pages.yml:12 |
| 9 | Infra | `FROM scratch` final Docker stage - no runtime libs (libc, OpenSSL certs) | Dockerfile.beatled:36 |

## HIGH

| # | Component | Issue | Location |
|---|-----------|-------|----------|
| 10 | Server | Missing size validation for UDP messages before type dispatch | udp_request_handler.cpp:20-40 |
| 11 | Pico | Pbuf freed immediately after `udp_sendto()` (lwip may still own it) | udp_sender.c:61 |
| 12 | Pico | Endianness macro has syntax issue (missing parentheses in conditions) | network.h:11 |
| 13 | Pico | DNS resolution can hang forever (no timeout) | dns.c:46 |
| 14 | Pico | State array out-of-bounds if state > 5 (no bounds check) | state_manager.c:50 |
| 15 | Pico | `serialize.c` has `msg_data.type` instead of `msg_data->type` - won't compile if used | serialize.c:12 |
| 16 | Client | No runtime validation on `.json()` API responses | api.ts, status.ts, program.ts, log.ts |
| 17 | Infra | Hardcoded IPs in cert script (192.168.1.5/6) | create-certs.sh:63-64 |
| 18 | Infra | No security scanning in CI (CodeQL, dependabot, container scanning) | .github/workflows/ |

## MEDIUM

| # | Component | Issue | Location |
|---|-----------|-------|----------|
| 19 | Server | Uninitialized `tempo_` and `time_ref_` in StateManager | state_manager.hpp:52-53 |
| 20 | Server | Callback vector iterated without lock in `update_next_beat()` | state_manager.cpp:30-34 |
| 21 | Server | Circular CMake dependency core <-> server | src/CMakeLists.txt |
| 22 | Server | Missing TLS 1.0/1.1 disabling (`no_tlsv1`, `no_tlsv1_1`) | http_server.cpp:121 |
| 23 | Server | Lock contention in audio callback (two lock acquisitions per buffer swap) | audio_interface.cpp:171 |
| 24 | Client | Array index used as key in log list rendering | log.tsx:71,95 |
| 25 | Client | API token stored in plain localStorage (XSS-vulnerable) | api.ts:14-24 |
| 26 | Client | `executeFetch` method param is `string` not union type | api.ts:26 |
| 27 | Client | Sourcemaps enabled in production build | vite.config.ts:61 |
| 28 | Pico | Inconsistent mutex usage (blocking vs try-lock) | ws2812.c:34 |
| 29 | Pico | Timer allocation not checked (alarm leak on failure) | tempo_synced.c:23 |
| 30 | Infra | Typo `sumbodule` in README and docs | README.md:56 |
| 31 | Infra | Typo `bealted-server` in vcpkg.json name field | vcpkg.json:2 |
| 32 | Infra | Port inconsistency in docs: 8080 vs 8443 | client.markdown, copy-tar-files.sh:50 |
| 33 | Infra | Docker uses `npm install` instead of `npm ci` | Dockerfile.beatled:33 |

## LOW

| # | Component | Issue | Location |
|---|-----------|-------|----------|
| 34 | Server | Commented-out code blocks (audio_input.hpp, CMakeLists) | audio_input.hpp:37-111 |
| 35 | Server | Unused OpenSSL find_package (found but not linked) | CMakeLists.txt:35-39 |
| 36 | Server | Dead TCP session code (no registration visible) | tcp_session.cpp |
| 37 | Client | No view-level component tests (status, program, config, log) | client/src/views/ |
| 38 | Client | Missing accessibility labels on Switch and form inputs | config.tsx, status.tsx |
| 39 | Infra | `.dockerignore` missing docs/, .git/, .vscode/ | .dockerignore |
| 40 | Infra | No EXPOSE or USER directives in Dockerfiles | docker/ |

## Summary

| Component | Critical | High | Medium | Low |
|-----------|----------|------|--------|-----|
| Server | 1 | 1 | 5 | 3 |
| Client | 0 | 1 | 4 | 2 |
| Pico | 5 | 5 | 2 | 0 |
| Infra | 3 | 2 | 4 | 2 |
| **Total** | **9** | **9** | **15** | **7** |
