# Unfixed Code Review Items (as of 2026-02-14)

Source: `docs/code-reviews/code-review-2026-02-13.md` (R1) and `docs/code-reviews/code-review-2-2026-02-13.md` (R2)

## CRITICAL (4)
- R1-1: Infra — vcpkg unpinned in Docker (`git checkout master`) vs pinned in CI
- R1-2: Infra — Pages workflow triggers on `feature/build-steps` not `master`
- R1-3: Infra — `FROM scratch` final Docker stage — no runtime libs
- R2-C6: Infra — Docker base images not pinned to digest

## HIGH (12)
- R1-4: Pico — Pbuf freed immediately after `udp_sendto()` (lwip may still own it)
- R1-5: Pico — Endianness macro missing parentheses
- R1-6: Pico — `serialize.c` uses `.` instead of `->` — won't compile if used
- R1-7: Client — No runtime validation on `.json()` API responses
- R1-8: Infra — Hardcoded IPs in cert script
- R1-9: Infra — No security scanning in CI (CodeQL, dependabot)
- R2-H3: Server — Missing JSON schema validation / body size limit
- R2-H4: Pico — Unsafe ISR alarm access without spinlock
- R2-H6: Client — CORS validation missing — `setAPIHost()` accepts any URL
- R2-H7: Client — XSS risk in log display (fragile pattern)
- R2-H8: Infra — Certificate script uses 2048-bit RSA
- R2-H9: Infra — No build artifact uploads in CI

## MEDIUM (30)
### Server (7)
- R1-10: Uninitialized `tempo_`/`time_ref_` in StateManager
- R1-11: Callback vector iterated without lock in `update_next_beat()`
- R1-12/R2-M7: Circular CMake dep core <-> server
- R1-13/R2-M3: Missing TLS 1.0/1.1 disabling
- R1-14: Lock contention in audio callback
- R2-M1: `is_running()` not const
- R2-M2: Redundant `std::move` defeats NRVO

### Pico (8)
- R2-M9: Unused ISR thread (dead code)
- R2-M10: Bit-shift message types not named constants
- R2-M11: `printf()` in ISR context
- R2-M12: Magic timing constants undocumented
- R2-M13: State machine no timeout/recovery
- R2-M14: No centralized state validation in command pipeline
- R2-M15: LED module tightly coupled to registry
- R2-M16: HAL queue mixes stack/heap across ports

### Client (10)
- R1-15: Array index as key in log list
- R1-16: API token in plain localStorage
- R1-17: `executeFetch` param is `string` not union
- R1-18: Sourcemaps in production
- R2-M17: Chart config recreated every render (no useMemo)
- R2-M18: Console interception unbounded
- R2-M19: Global mutable state (`API_HOST`, `tempoHistory`)
- R2-M20: API layer no error classification/retry/timeout
- R2-M21: Loader modifies global state
- R2-M22: Missing CSP headers

### Infra (5)
- R1-19: Port inconsistency in docs (8080 vs 8443)
- R1-20: `npm install` instead of `npm ci` in Docker
- R2-M28: Docker APT cache not cleaned
- R2-M31: `.dockerignore` missing entries
- R2-M40: vcpkg no version constraints

## LOW (14)
- R1-21: Commented-out code blocks
- R1-22: Unused OpenSSL find_package
- R1-23: Dead TCP session code
- R1-24/R2-L6: No view-level tests / no lazy loading
- R1-25/R2-M25: Missing accessibility labels
- R1-26/27: .dockerignore missing, no EXPOSE/USER in Dockerfiles
- R2-L1: String param copies in APIHandler constructor
- R2-L2: Inconsistent namespace org
- R2-L3: Outdated clang-tidy comments
- R2-L4: Busy-wait DNS resolution
- R2-L7: Deprecated ESLint rules
- R2-L8: Missing `resolveJsonModule`
- R2-L9: No bundle size analysis
- R2-L12: Ruby bundler old
