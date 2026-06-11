# Code review — whole-repo sweep (2026-06-11)

Scope: server, controller firmware (incl. the then-uncommitted
`fix/pico-priu64-format` working tree), React client, iOS app, scripts,
CI workflows, and docs. Findings were cross-checked against
`docs/code-reviews/code-review-unfixed.md` (2026-02-14); items below are
**new** unless they explicitly reference an R1/R2 number. The earlier
backlog is not repeated here — see [N14](#low).

## Resolution status (2026-06-11)

All of N1–N14 are resolved. ✅ = fixed and verified in-tree; the per-row
notes below point at the landing evidence. The consolidated tracker is
`docs/code-reviews/code-review-unfixed.md`.

## Overall assessment

The codebase is in good shape. Highlights worth keeping as-is:

- Server: clean service decomposition (BeatDetector, ManualTempo,
  UDPServer, HTTPServer, TempoBroadcaster) behind a ServiceManager, ASIO
  strands for serialization, per-client OWD compensation, protocol
  version gating, bearer-token auth with rate limiting, TLS by default,
  path-traversal protection in the file handler.
- Controller: thorough HAL abstraction across five ports, single
  source-of-truth wire protocol with exact-size validation on every
  inbound message and struct-size assertions in
  `controller/tests/posix/protocol/test_protocol.cpp`.
- Client: token in `sessionStorage` (deliberate), API access funnelled
  through two wrappers, `AbortController` cleanup in health checks.
- iOS: `KeychainTokenStore` is well designed (debug/release service
  tags, legacy-UserDefaults migration).
- Scripts/hooks: `beatled.sh` help coverage is complete;
  `scripts/git-hooks/pre-commit` is bash-3.2-safe, NUL-delimited, and
  degrades gracefully when linters are missing.

## High

| # | Status | Area | Finding | Suggested fix |
|---|--------|------|---------|---------------|
| N1 | ✅ Fixed | iOS | Zero tests anywhere under `ios/` — no ViewModel, APIClient, or KeychainTokenStore coverage. | Add an XCTest target; start with APIClient decoding and KeychainTokenStore save/delete semantics. (BeatledTests target; model/Keychain/AppSettings/ConfigVM tests.) |
| N2 | ✅ Fixed | CI | iOS is never built in CI — `.github/workflows/` covers server, client, controller, docs only. Build breakage goes undetected. | Add an `ios-ci.yml` job on a macOS runner: `xcodebuild build` (and `test` once N1 lands), triggered on `ios/**` changes. (`.github/workflows/ios-ci.yml`, macos-15 + Xcode 26.) |
| N3 | ✅ Fixed | Server | `server/src/server/udp_server/udp_request_handler.cpp:83` (also :137, :155, :185): `reinterpret_cast` of the raw UDP receive buffer into packed protocol structs without an alignment guarantee — technically UB. | `memcpy` the payload into a properly aligned local struct after the size check. (memcpy in all four handlers.) |
| N4 | ✅ Fixed | Controller | Time sync has no timeout on the outstanding-request flag (`controller/src/command/time/time.c:44-45`); a lost TIME_RESPONSE leaves the controller waiting instead of re-requesting. | Stamp the request time and clear/resend after a timeout (e.g. 2× median RTT, floor 500 ms). (1 s retry timers owned by REGISTERED and TIME_SYNCED.) |

## Medium

| # | Status | Area | Finding | Suggested fix |
|---|--------|------|---------|---------------|
| N5 | ✅ Fixed | Server | `server/src/server/http/file_handler.cpp:41-45` swallows every exception into an unlogged 404 — masks permission and IO errors. | Log the exception message at WARN; return 404 vs 500 based on error type. (404 only for missing files; everything else logged WARN + 500.) |
| N6 | ✅ Fixed | Server tests | TempoBroadcaster (OWD compensation, 50 ms program-push retries) has no tests; HTTPServer TLS setup is also untested. | Unit-test broadcaster scheduling/compensation with a fake clock and socket; smoke-test cert loading. (`test_tempo_broadcaster`, `test_tls_context`.) |
| N7 | ✅ Fixed | Controller tests | No automated LED-pattern tests — `controller/tests/ws2812/main.c` is compile-only. The new `off` and rewritten `snake` patterns landed untested. | Add a POSIX Catch2 test: `off` ⇒ all pixels zero; `snake` head advances one full lap per `SNAKE_BEATS_PER_LOOP` beats. (`controller/tests/posix/patterns/test_patterns.cpp`.) |
| N8 | ✅ Fixed | Controller | Hello path (`controller/src/command/hello/hello.c`) copies `board_id` into the 17-byte wire field without an explicit bound/termination check. | Use a bounded copy and assert/truncate explicitly. (Zero-fill + `_Static_assert` bound.) |
| N9 | ✅ Fixed | Server | Program names ("Snakes!", …) hardcoded at `server/src/server/http/api_handler.cpp:330-333`; adding a firmware pattern (e.g. `off`) requires remembering to mirror it here. | Move the list to one shared definition (config file or generated from the firmware pattern table). (`BEATLED_PROGRAM_TABLE` X-macro in the shared protocol header.) |
| N10 | ✅ Fixed | Client | All API failures collapse to `{error: true, status: "Network error"}` (`client/src/lib/status.ts:18`, `program.ts:26`, `log.ts:13`) — HTTP status never surfaces. Extends open item R2-M20. | Distinguish network vs HTTP errors and carry the status code through to the views. (Closed with R2-M20: `ApiError` kinds http/network/timeout/invalid.) |

## Low

| # | Status | Area | Finding | Suggested fix |
|---|--------|------|---------|---------------|
| N11 | ✅ Fixed | Client tests | 2 of 5 views untested (log, config main logic). | Add view tests mirroring the existing status/program ones. (log + config view tests; all five views covered.) |
| N12 | ✅ Fixed | Server | BPM accepted as `double`, silently narrowed to `float` (`server/src/server/http/api_handler.cpp:220-230`). | Validate range, then narrow explicitly; the precision loss is fine, the silence isn't. (Validated 20–400, explicit cast.) |
| N13 | ✅ Resolved | Hygiene | Branch `fix/pico-priu64-format` carries unrelated work (snake rewrite, off pattern); the name will mislead later archaeology. | Commit the pattern work under its own subject (or branch) when landing. (Pattern work landed on master under its own subject `c04a4f7`.) |
| N14 | ✅ Fixed | Process | `code-review-unfixed.md` is four months stale; its 4 critical infra items (Pages workflow trigger, unpinned vcpkg in Docker, `FROM scratch` runtime, unpinned base images) may or may not still be open. | Triage pass: mark each item fixed/still-open and re-date the doc. (Re-triaged and re-dated 2026-06-11.) |

## Verified good (no action)

- `/api/qos` **is** documented (`docs/api.markdown:330`) — an initial
  finding to the contrary was checked and found wrong.
- Auth, rate limiting, TLS-by-default, path-traversal protection, and
  the sessionStorage/Keychain token split are all sound.
- The uncommitted snake/off diff: guards `len == 0` before the modulo
  math, fixed-point (Q8) arithmetic stays in range
  (`beat_env` ∈ [40, 254]), and the pattern-table/CMake/header wiring is
  complete. Safe to merge once N7's test exists.

## Suggested first batch

N5 (file-handler logging) + N3 (memcpy) on the server, N7 (off-pattern
test) on the controller, and N2 (iOS CI job).
