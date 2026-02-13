# Changelog

## Unreleased

### Bug Fixes (beatled-pico)
- Fix critical bitwise operator bug in `ws2812.c` — used `|` (OR) instead of `&` (AND) for inter-core message type checks, causing all message branches to execute unconditionally
- Remove dead code (`pattern_idx = pattern_idx`) in `ws2812_patterns.c`
- Fix state machine missing `STATE_TEMPO_SYNCED` case in transition switch — previously fell through to "Unknown state" error
- Allow `TEMPO_SYNCED → TEMPO_SYNCED` re-sync transition in state machine
- Add malloc NULL checks across 8 files (UDP server, queues, alarms, state manager, autotest)

### CI/CD
- Add C++ server CI pipeline (CMake + vcpkg + Catch2 tests on Ubuntu)
- Add React client CI pipeline (npm + Vite build)
- Pin all GitHub Actions to SHA hashes for supply-chain security

### Code Quality
- Add `.clang-format` config for consistent C++ formatting
- Add ESLint + Prettier config for React client
- Make CORS `Access-Control-Allow-Origin` configurable via `--cors-origin` CLI flag (defaults to `*`)

### Testing
- Expand `StateManager` tests: tempo updates, program ID, next-beat callbacks, client re-registration, and thread safety

### Documentation
- Add architecture page with system diagram, protocol documentation, and component descriptions
