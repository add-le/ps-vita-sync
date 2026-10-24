# Plans — Roadmap

How to get from the current code ([../README.md](../README.md)) to the
[vision](../vision.md). Phases are ordered by dependency; each has an exit gate that must
be met before work that depends on it starts.

Only Phase 0 and Phase 1 have detailed plans. Later phases list goals and open questions
only: their shape depends on what the Phase 1 spikes find, and a task list written before
those results would be guesswork. Each later phase gets a detailed plan when its
prerequisites are met.

## Overview

```
Phase 0  Foundations ─────────────┐
                                  ├─→ Phase 2  Core ─→ Phase 3  Providers ─┐
Phase 1  Spikes (parallel) ───────┘        │                              ├─→ Phase 5  v1.0
           UI stack · native saves         └─→ Phase 4  App & Vita features ┘
           network/TLS · game-close plugin
           credential storage
```

Phase 0 and Phase 1 can run at the same time. Spikes write their results into their own
plan file, under a "Results" section.

## Phase 0 — Foundations

Make every later change verifiable by agents and CI.

- Plan: [phase-0-foundations.md](phase-0-foundations.md)
- Exit gate: `cmake` builds a portable core library and its unit tests on Linux; CI builds
  the VPK and runs the tests on every push; an e2e test runs against a WebDAV container.

## Phase 1 — Research spikes (parallel)

Answer the questions the architecture depends on. Most need a real Vita.

| Spike | Question | Hardware? | Plan |
|---|---|---|---|
| UI stack | Which HTML/CSS stack can deliver the UI within Vita limits? | Yes | [spike-ui-stack.md](spike-ui-stack.md): all stacks built, host results in, RmlUi leading; hardware run pending |
| Native saves | Can the app read and write native Vita saves, and how? | Yes | [spike-native-saves.md](spike-native-saves.md) |
| Network & TLS | Do the system SSL stack and CA store work with today's provider endpoints? | Yes | [spike-network-tls.md](spike-network-tls.md) |
| Game-close plugin | Can a plugin detect a game closing and trigger a sync? | Yes | [spike-game-close-plugin.md](spike-game-close-plugin.md) |
| Credential storage | How to derive a device-bound key, and what it protects | Partly | [spike-credential-storage.md](spike-credential-storage.md) |
| Providers | Auth, scopes, APIs and limits for each candidate provider | No | [providers.md](providers.md) |

Exit gate: each spike has a written result and a decision; [../vision.md](../vision.md)
is updated where a spike changes an assumption.

## Phase 2 — Core (goals only)

- Provider interface: list, stat, download, upload (resumable), create folder, delete,
  move, revision/hash, change feed where available, quota.
- Sync engine: per-item state (last synced hash, size, remote revision), change
  detection on both sides, the two-way decision table, conflict records for the UI.
- State store: sqlite or a manifest format (decide here).
- Local backup before every overwrite; atomic writes (temp file, verify, rename).
- Job queue: retries, rate-limit backoff, partial failure (one failed item never stops the
  rest).
- Settings and encrypted credential store (from the credential spike).
- Logging to file under `ux0:/data/SYNC00055/`.

Open questions: state store format; how to hash large files quickly on the Vita;
whether the sync engine runs on a worker thread alongside the UI.

## Phase 3 — Providers (goals only)

Order: **WebDAV first** (the only one fully testable end-to-end in a container), then
**Google Drive** (partly built), then the rest by priority from
[providers.md](providers.md). Each provider ships with contract tests against its fake
and an optional live smoke test.

## Phase 4 — App & Vita features (goals only)

UI built on the chosen stack; game-aware save list; conflict and restore screens; sync
triggers (launch, manual, scheduled, LiveArea quick-sync, game-close plugin); cloud
history; native save support (per spike result); i18n with system-language detection and
CJK font; in-app updater.

## Phase 5 — v1.0

Everything in the vision, released through GitHub Releases and VitaDB.

## Pre-releases

0.x builds are published as milestones complete so they can be tested on real Vitas.
Suggested cut points: first provider syncing end-to-end with the new UI; every further
provider; native saves; plugin.