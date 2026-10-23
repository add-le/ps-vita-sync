# Phase 0 — Foundations

**Goal:** every later change can be built and tested by an agent or CI without a Vita.
Today nothing can be compiled outside a VitaSDK install, and nothing is tested.

## Deliverables

1. **Source tree split**

   ```
   core/        portable code: sync engine, providers, state store, parsers, crypto
                (no psp2 headers; platform calls go through interfaces)
   platform/    platform implementations
     vita/      SceIo, SceHttp/curl, SceAppUtil, input, rendering
     host/      POSIX equivalents for tests
   app/         Vita application: main, UI, wiring
   tests/
     unit/      host-only, no network
     e2e/       Testcontainers-driven
   legacy/ or the current src/   kept until its useful parts are ported (Phase 2)
   ```

   The exact names are a proposal; settle them in this phase and record them in
   [../architecture.md](../architecture.md).

2. **CMake**
   - A `core` library target that compiles with both the VitaSDK toolchain and the host
     compiler.
   - `app` target (VPK) only when the Vita toolchain is active.
   - `tests` targets only on host builds, behind `BUILD_TESTING`.
   - Keep the current manual source lists, or switch to per-directory lists; decide and
     document.

3. **Language and standard for new code** — decide and record:
   - C11 vs C++ for the core. The UI candidates (RmlUi, litehtml) are C++, and the
     existing C++ flags are `-std=c++11 -fno-rtti -fno-exceptions`.
   - **Verify** which C++ standard the current VitaSDK GCC supports, and whether
     exceptions/RTTI can be enabled if a UI library needs them.
   - Error handling: return codes everywhere; no `logger_exit` outside startup.

4. **Unit test framework** — pick one that works for the chosen language (for example
   Unity or cmocka for C; doctest or Catch2 for C++). Criteria: header-only or tiny,
   runs on host, CTest integration.

5. **End-to-end tests with Testcontainers** — three layers:

   | Layer | What | In CI |
   |---|---|---|
   | Real servers | Self-hostable services in containers: WebDAV (e.g. a Nextcloud or plain WebDAV server image), S3-compatible if adopted | Yes |
   | API fakes | Contract-faithful HTTP fakes for SaaS APIs (Drive, Dropbox, OneDrive, …), run as containers or in-process | Yes |
   | Live smoke | Real accounts with credentials from CI secrets or a developer machine | No (manual / opt-in) |

   **Verify** a Testcontainers library usable from the chosen test language (e.g.
   testcontainers for C/C++ is less mature than for Go/Java/Python/Node; a driver script in
   another language that starts containers and runs the C/C++ test binary is an
   acceptable fallback).

6. **CI (GitHub Actions)**
   - Job 1: host build + unit tests + e2e tests (Docker available on GitHub runners).
   - Job 2: VPK build in a VitaSDK container (**Verify** a maintained VitaSDK Docker
     image) and upload the VPK as an artifact.
   - Formatting check (clang-format) once a `.clang-format` is committed.

7. **Docs** — update [../build-and-release.md](../build-and-release.md),
   [../conventions.md](../conventions.md) and `CLAUDE.md` with the new layout, commands and
   test instructions.

## First code to move

To prove the split works end to end, move one small, already-portable piece into `core/`
with tests, for example base64url + SHA-256 + RS256 + JWT (fixing the bugs listed in
`CODE_REVIEW.md` #10, #13 and the base64 padding issue while doing it). These depend only
on OpenSSL and are testable against known vectors.

## Exit gate

- `cmake -B build-host && cmake --build build-host && ctest --test-dir build-host` passes
  on Linux.
- CI is green on a PR: host tests, one e2e test against a WebDAV container, VPK artifact.
- An agent in the cloud container can run the host build and tests.

## Not in this phase

No provider implementation, no UI work, no sync engine. Those wait for Phase 1 results.