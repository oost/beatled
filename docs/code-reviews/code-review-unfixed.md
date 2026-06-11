# Unfixed Code Review Items (as of 2026-06-11)

Sources: `docs/code-reviews/code-review-2026-02-13.md` (R1),
`docs/code-reviews/code-review-2-2026-02-13.md` (R2), and
`docs/code-reviews/code-review-2026-06-11.md` (N1–N14).

Re-triaged and re-dated 2026-06-11 [N14]. The 2026-06 review-fixes batch
(branch `feature/review-fixes-2026-06`) closed everything that was still
open except the deliberate-deferral list at the bottom. Items fixed in
that batch are marked ✗ (fixed); items that turned out to be already
fixed or wrong are marked with the reason.

## CRITICAL (0 open)

- ~~R1-1: vcpkg unpinned in Docker~~ — fixed earlier (Dockerfile.builder pins
  the same commit as CI)
- ~~R1-2: Pages workflow triggers on `feature/build-steps`~~ — fixed earlier
  (pages.yml triggers on `master`)
- ~~R1-3: `FROM scratch` final Docker stage~~ — fixed 2026-06 (test-docker
  vanilla image now uses the digest-pinned debian runtime pattern)
- ~~R2-C6: Docker base images not pinned to digest~~ — fixed 2026-06 (all
  FROM lines are `tag@sha256:…`)

## HIGH (0 open)

- ~~R1-4: pbuf freed after `udp_sendto()`~~ — already correct (lwIP send is
  synchronous-copy; free on return path is the documented pattern)
- ~~R1-5: endianness macro missing parentheses~~ — fixed 2026-06 (htonll
  fully parenthesizes + explicit 64→32 truncation)
- ~~R1-6: broken `serialize.c`~~ — removed 2026-06 (dead code, never built)
- ~~R1-7: no runtime validation on API responses~~ — fixed 2026-06 (type
  guards on every `.json()` in the client lib)
- ~~R1-8: hardcoded IPs in cert script~~ — fixed earlier (mkcert + domain
  parameters)
- ~~R1-9: no security scanning in CI~~ — fixed 2026-06 (codeql.yml:
  js-ts + c-cpp; dependabot.yml existed already)
- ~~R2-H3: missing body size limit~~ — fixed earlier (4096-byte cap on all
  POST endpoints)
- ~~R2-H4: unsafe ISR alarm access~~ — fixed 2026-06 (interrupt-disabled
  critical sections around `reset_delay_alarm_id` in both Pico DMA ports)
- ~~R2-H6: `setAPIHost()` accepts any URL~~ — fixed 2026-06 (URL parse +
  http/https scheme check)
- ~~R2-H7: XSS risk in log display~~ — already fixed (plain `<pre>` text,
  no `dangerouslySetInnerHTML` anywhere)
- ~~R2-H8: 2048-bit DH params~~ — fixed 2026-06 (4096)
- ~~R2-H9: no build artifact uploads in CI~~ — fixed earlier (all three CI
  workflows upload failure logs)

## MEDIUM (0 open)

Server: R1-10 (member init), R1-11 (callback registration documented as
construction-only), R1-12/R2-M7 (core↔server cycle broken in #54),
R1-13/R2-M3 (TLS 1.0/1.1 disabled), R1-14 (audio callback is lock-free),
R2-M1 (`is_running()` const), R2-M2 (no `return std::move` left) — all
verified fixed or not applicable.

Pico: ~~R2-M9 unused ISR thread~~ — removed 2026-06 (call site, no-op
entry, and the HAL hook across all five ports). ~~R2-M10 raw bit-shift
message types~~ — fixed 2026-06 (`INTERCORE_FLAG_*`). ~~R2-M11 printf in
ISR~~ — already commented out. ~~R2-M12 magic 400 µs~~ — fixed 2026-06
(`WS2812_RESET_DELAY_US` + datasheet note). ~~R2-M13 state machine no
timeout~~ — fixed 2026-06 for the cases that mattered (REGISTERED and
TIME_SYNCED re-send their request every 1 s until answered, see also N4;
TEMPO_SYNCED already had refresh timers).

Client: ~~R2-M17 chart useMemo~~, ~~R2-M18 console buffer bound~~,
~~R1-15 array-index keys~~, ~~R1-16/localStorage token~~ (sessionStorage
is the deliberate design), ~~R2-M21 loader global state~~ (documented
ring buffer) — already fixed. ~~R2-M20 error classification~~ — fixed
2026-06 (`ApiError` kinds http/network/timeout/invalid, 10 s timeout,
HTTP status surfaced to views; also closes N10). ~~R2-M22 CSP~~ — fixed
2026-06 (meta tag in the production build, stripped under `vite dev`).
~~R1-17 endpoint union type~~ — fixed 2026-06.

Infra: ~~R1-19 port docs~~ — fixed 2026-06 (last two 8080 strings in
beatled.sh help). ~~R1-20 npm ci~~ — fixed earlier. ~~R2-M28 apt cache~~ —
fixed 2026-06. ~~R2-M31 .dockerignore~~ — fixed 2026-06 (build dirs +
client/dist).

## LOW (0 open)

~~R1-21 commented-out code~~ — last instance removed 2026-06.
~~R1-22 unused find_package(OpenSSL)~~ — wrong: OpenSSL::SSL/Crypto are
linked by the http target; the find_package stays.
~~R1-23 dead TCP session code~~ — removed 2026-06 (never referenced by
any CMakeLists).
~~R1-24/R2-L6 view tests~~ — log + config view tests added 2026-06 (N11);
all five views covered now.
~~R2-L8 resolveJsonModule~~ — fixed 2026-06.
R1-25/R2-M25 a11y labels, R2-L1/L2/L3/L4, R2-L7 — verified fixed or not
applicable on inspection.

## June 2026 review (N1–N14)

- ~~N1 iOS tests~~ — fixed 2026-06 (BeatledTests target, 29 tests:
  model decoding, KeychainTokenStore, AppSettings migration, ConfigVM)
- ~~N2 iOS CI~~ — fixed 2026-06 (ios-ci.yml, macos-15 + Xcode 26)
- ~~N3 UDP reinterpret_cast UB~~ — fixed 2026-06 (memcpy into aligned
  locals in all four handlers)
- ~~N4 time-sync no timeout~~ — fixed 2026-06 (1 s retry timers owned by
  the REGISTERED and TIME_SYNCED states)
- ~~N5 file-handler swallowed exceptions~~ — fixed 2026-06 (404 only for
  missing files; everything else logged WARN + 500)
- ~~N6 broadcaster/TLS untested~~ — fixed 2026-06
  (test_tempo_broadcaster: 5 cases incl. the 50 ms retry and status
  probes; test_tls_context smoke-tests cert loading)
- ~~N7 LED pattern tests~~ — fixed 2026-06 (test_patterns: off blanks,
  snake lap timing, table/protocol sync)
- ~~N8 hello board_id bound~~ — fixed 2026-06 (zero-fill +
  `_Static_assert` bound)
- ~~N9 hardcoded program names~~ — fixed 2026-06 (`BEATLED_PROGRAM_TABLE`
  X-macro in the shared protocol header; also fixed the "Fade Color/s"
  drift it found)
- ~~N10~~ — closed with R2-M20 above
- ~~N11~~ — closed with R1-24 above
- ~~N12 BPM narrowing~~ — was already fixed by the manual-tempo commit
  (validated 20–400, explicit cast)
- N13 branch hygiene — moot: the pattern work landed on master under its
  own subject (`c04a4f7`)
- ~~N14 stale backlog~~ — this document

## Deliberately deferred (accepted, not planned)

- R2-M14: fully centralized field-level validation in the command
  pipeline — size validation is centralized; per-field checks stay in
  handlers. Revisit only if a real bug shows up here.
- R2-M15: LED module / registry decoupling — works, low churn, refactor
  not worth the risk until the LED module next changes shape.
- R2-M16: HAL queue allocation strategy differs per port (`malloc` vs
  `pvPortMalloc`) — this is the correct idiom per RTOS, not an
  inconsistency to fix.
- R2-M19: `API_HOST` module-level state in the client — bounded, single
  writer, validated on write since 2026-06.
- R2-M40: vcpkg per-package version overrides — the builtin-baseline pin
  is the accepted strategy.
- R2-L6 (lazy loading), R2-L9 (bundle size analysis), R2-L12 (bundler
  version): nice-to-haves with no current pain.
