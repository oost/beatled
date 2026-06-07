# Beatled — contributor & assistant conventions

This file codifies the project's working conventions so contributors and
AI assistants (Claude Code, etc.) follow the same patterns. It's a
small, scannable document — if a section grows past one screen, split it
out into `docs/`.

## Repository layout

The whole product lives in this one repo (since the `beatled-pico`
submodule was absorbed into `controller/`):

```
server/      C++ beat server (asio + restinio + spdlog)
client/      React + Vite + shadcn/ui
ios/         SwiftUI app shared by iOS + macOS
controller/  Embedded firmware for Pico W / ESP32 + POSIX simulator
docs/        Jekyll site published at oost.github.io/beatled
scripts/     beatled.sh (build / test / deploy CLI) + git-hooks/
.github/     CI workflows and dependabot config
```

External code we own a fork of stays as a submodule under
`server/external/` (currently `beatled-beat-tracker`). New work goes
in-tree; resist adding new submodules.

## Build / test / lint

A single script wraps the cross-component flows:

```sh
scripts/beatled.sh server build       # cmake + ninja under server/build/
scripts/beatled.sh server start --start-http --start-udp --start-broadcast
scripts/beatled.sh client             # vite dev server with /api proxy
scripts/beatled.sh controller pico build       # uf2 for Pico W bare-metal
scripts/beatled.sh controller pico-freertos build
scripts/beatled.sh controller pico flash       # build + copy to /Volumes/RPI-RP2
scripts/beatled.sh controller posix build      # native simulator (POSIX port)
scripts/beatled.sh test server                 # catch2 server tests
scripts/beatled.sh test pico                   # catch2 firmware tests (POSIX)
scripts/beatled.sh test all
scripts/beatled.sh clean all
```

Tests live next to the code they exercise:

- `server/tests/{api_handler,state_manager,tempo_broadcaster,udp,...}/`
- `controller/tests/posix/{clock,event_queue,integration,protocol,...}/`
- `client/src/**/__tests__/`

Catch2 binaries on the firmware side aren't registered with `ctest`;
invoke each one directly (the `Run unit tests` step in
`.github/workflows/controller-ci.yml` shows the pattern).

## Local hooks

```sh
scripts/git-hooks/install.sh
```

Wires `core.hooksPath` to `scripts/git-hooks/`, which runs
`clang-format` on staged C / C++ and `shellcheck` on staged
`scripts/*.sh`. Each tool is optional — if it isn't on PATH the hook
prints a skip notice and moves on. Use `BEATLED_SKIP_HOOKS=1 git
commit …` to bypass for emergencies; don't make a habit of it.

## Code review docs

Long-form architectural reviews, audits, and rollout plans go under
`docs/code-reviews/` with the date in the filename
(e.g. `docs/code-reviews/code-review-2026-02-13.md`). The active plan
file from an AI assistant is `~/.claude/plans/…` per-session and is
*not* committed; once accepted, the actionable parts should move into a
dated review doc under `docs/code-reviews/`.

## Wire protocol

The single source of truth for the server↔controller UDP protocol is
`controller/lib/beatled_protocol/include/beatled/protocol.h`. The
server's `server/CMakeLists.txt` resolves `BEATLED_PROTOCOL_PATH` to
that same path. Changing the wire format means changing one header and
rebuilding everything — there's no parallel copy to keep in sync any
more, by design.

The protocol is documented narratively in
`docs/controller-sync.markdown` and at the byte level in
`docs/code-reviews/code-review-2-2026-02-13.md`.

## Commit conventions

- Subject line: present tense, < 70 chars, no trailing period. Imperative
  voice ("Add foo", not "Added foo" or "Adds foo").
- Body: explain the *why*, not the *what* (the diff already shows the
  what). Hard-wrap at 72 columns where practical. Reference review
  findings as `[F4]` / `[A6]` etc. when applicable.
- Co-author trailer for AI-assisted commits:
  `Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>`
- Bundle small Tier-1-style changes (gitignore, log levels, docs typos)
  into a single themed commit rather than one commit per file.
- One protocol bump per commit; never mix protocol changes with code
  refactors.

## Branching / PRs

- Trunk: `master`. Feature branches: `feature/<short-slug>`. CI is
  pinned to those two patterns; a `main` branch will not trigger
  workflows.
- Default to a PR even on solo work — the PR description is the
  artefact that survives once the branch is merged. The plan file used
  to design the change is a useful body for the PR.
- Merge style: keep history linear (`git merge --ff-only` or rebase),
  but don't force-push to `master`. Pull-request rebases on the feature
  branch are fine.

## API stability

The HTTP surface (`/api/status`, `/api/program`, `/api/devices`,
`/api/health`, `/api/service-control`, `/api/log*`) is consumed by
three clients (React, iOS, macOS). When changing a response shape:

1. Update the server first.
2. Update `docs/api.markdown` in the same commit.
3. Update the React and Swift consumers in immediately-following
   commits.

`devices[].last_status_time` is the canonical field name; `last_seen`
is the historical mistake the schema-drift Tier-1 fix renamed away.
The other consumer-facing identifiers are documented in
`docs/api.markdown`.

## Security baseline

- API tokens belong in the iOS Keychain (`Services/KeychainTokenStore.swift`),
  not `UserDefaults`. The React client uses `sessionStorage` — that's a
  deliberate choice, not laziness, so a public computer doesn't leak
  the token across sessions.
- TLS is on by default. `--no-tls` exists for local development and
  logs a `SPDLOG_WARN` on startup so it can't sneak into production
  unnoticed. Never deploy with it.
- `.env` files are gitignored at the root. Wi-Fi passwords, API
  tokens, and any other secret-shaped string belong in there. Commit
  the matching `.env.*.template` so onboarding still works.
- The server's `--api-token` flag should be set in any deployment a
  third party can reach. The pre-shared token is checked on every
  state-changing endpoint.

## Working with AI assistants

A few things that have repeatedly bitten us:

- Submodule edits: there's only one firmware tree now
  (`controller/`). If you find yourself editing
  `server/external/beatled-pico/…` something is wrong — that path no
  longer exists.
- Commit hygiene: review staged files with `git diff --cached` before
  committing. The assistant occasionally stages files outside the
  task's scope (build artefacts, `.DS_Store`, etc.); these are caught
  by `.gitignore` if it's complete, but spot-check anyway.
- POSIX integration tests are the fast feedback loop for firmware
  changes. Run them (`scripts/beatled.sh test pico`) before declaring
  a firmware change done — the per-Pico-W flash cycle is much slower.
