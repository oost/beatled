# Code Review - 2026-02-13

## CRITICAL

| # | Component | Issue | Location |
|---|-----------|-------|----------|
| 1 | Infra | vcpkg unpinned in Docker (`git checkout master`) vs pinned in CI | Dockerfile.builder:31 |
| 2 | Infra | Pages workflow triggers on `feature/build-steps` not `master` | pages.yml:12 |
| 3 | Infra | `FROM scratch` final Docker stage - no runtime libs (libc, OpenSSL certs) | Dockerfile.beatled:36 |

## HIGH

| # | Component | Issue | Location |
|---|-----------|-------|----------|
| 4 | Pico | Pbuf freed immediately after `udp_sendto()` (lwip may still own it) | udp_sender.c:61 |
| 5 | Pico | Endianness macro has syntax issue (missing parentheses in conditions) | network.h:11 |
| 6 | Pico | `serialize.c` has `msg_data.type` instead of `msg_data->type` - won't compile if used | serialize.c:12 |
| 7 | Client | No runtime validation on `.json()` API responses | api.ts, status.ts, program.ts, log.ts |
| 8 | Infra | Hardcoded IPs in cert script (192.168.1.5/6) | create-certs.sh:63-64 |
| 9 | Infra | No security scanning in CI (CodeQL, dependabot, container scanning) | .github/workflows/ |

## MEDIUM

| # | Component | Issue | Location |
|---|-----------|-------|----------|
| 10 | Server | Uninitialized `tempo_` and `time_ref_` in StateManager | state_manager.hpp:52-53 |
| 11 | Server | Callback vector iterated without lock in `update_next_beat()` | state_manager.cpp:30-34 |
| 12 | Server | Circular CMake dependency core <-> server | src/CMakeLists.txt |
| 13 | Server | Missing TLS 1.0/1.1 disabling (`no_tlsv1`, `no_tlsv1_1`) | http_server.cpp:121 |
| 14 | Server | Lock contention in audio callback (two lock acquisitions per buffer swap) | audio_interface.cpp:171 |
| 15 | Client | Array index used as key in log list rendering | log.tsx:71,95 |
| 16 | Client | API token stored in plain localStorage (XSS-vulnerable) | api.ts:14-24 |
| 17 | Client | `executeFetch` method param is `string` not union type | api.ts:26 |
| 18 | Client | Sourcemaps enabled in production build | vite.config.ts:61 |
| 19 | Infra | Port inconsistency in docs: 8080 vs 8443 | client.markdown, copy-tar-files.sh:50 |
| 20 | Infra | Docker uses `npm install` instead of `npm ci` | Dockerfile.beatled:33 |

## LOW

| # | Component | Issue | Location |
|---|-----------|-------|----------|
| 21 | Server | Commented-out code blocks (audio_input.hpp, CMakeLists) | audio_input.hpp:37-111 |
| 22 | Server | Unused OpenSSL find_package (found but not linked) | CMakeLists.txt:35-39 |
| 23 | Server | Dead TCP session code (no registration visible) | tcp_session.cpp |
| 24 | Client | No view-level component tests (status, program, config, log) | client/src/views/ |
| 25 | Client | Missing accessibility labels on Switch and form inputs | config.tsx, status.tsx |
| 26 | Infra | `.dockerignore` missing docs/, .git/, .vscode/ | .dockerignore |
| 27 | Infra | No EXPOSE or USER directives in Dockerfiles | docker/ |

## Summary

| Component | Critical | High | Medium | Low |
|-----------|----------|------|--------|-----|
| Server | 0 | 0 | 5 | 3 |
| Client | 0 | 1 | 4 | 2 |
| Pico | 0 | 2 | 0 | 0 |
| Infra | 3 | 2 | 2 | 2 |
| **Total** | **3** | **5** | **11** | **7** |

## Fixed Issues (removed from this review)

The following issues were resolved:

- ~~#1 Server: Unsafe reinterpret_cast without bounds check~~ — size validation added
- ~~#2 Pico: Use-after-free in command dispatch~~ — event_data freed safely after all usage
- ~~#3 Pico: exit() calls on non-fatal errors~~ — replaced with error return codes
- ~~#4 Pico: Race conditions on LED globals~~ — snapshot-under-lock pattern added
- ~~#5 Pico: Missing bounds validation in protocol parsing~~ — size checks added to all handlers
- ~~#6 Pico: UDP socket never closed~~ — shutdown_udp_socket() added
- ~~#10 Server: Missing size validation for UDP messages~~ — validation added before dispatch
- ~~#13 Pico: DNS resolution hang~~ — 30-second timeout added
- ~~#14 Pico: State array out-of-bounds~~ — bounds check on transition_matrix access
- ~~#28 Pico: Inconsistent mutex usage~~ — consistent lock pattern in ws2812.c
- ~~#29 Pico: Timer allocation not checked~~ — allocation checked with cleanup on failure
- ~~#30 Infra: Typo "sumbodule"~~ — corrected
- ~~#31 Infra: Typo "bealted-server"~~ — corrected
