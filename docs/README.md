# PS Vita Sync — Technical Documentation

Reference docs for humans and coding agents working on this repository. They describe
the code **as it is**, including what is broken, unfinished or disconnected, so that
nobody builds on an assumption the code does not support.

> Docs describe behavior; `CODE_REVIEW.md` (repo root) is the itemized bug list with
> severities. When the two overlap, the docs link to the review instead of repeating it.

## State of the app in one screen

- **What it is:** a PS Vita homebrew (`ps_vita_sync.vpk`, title ID `SYNC00055`, v`00.02`)
  meant to sync game save files between the Vita and Google Drive. C/C++, VitaSDK, CMake.
- **What actually runs today:** almost nothing. `main()` initializes the logger, OpenSSL
  and the network stack, calls `loadOAuthCredentials()` (which reads `config.json` and
  **prints the client id and client secret on screen**), prints "Sync correctly done" and
  exits after 3 s. No sync happens. See [architecture.md](architecture.md#the-live-path).
- **What worked once:** a hardcoded prototype (now commented out in `main.cpp`) that synced
  one PSP game save (`ULUS10391`, Monster Hunter Freedom Unite) under `ux0:/pspemu/`,
  authenticating with a Google service-account JWT. See [drive-sync.md](drive-sync.md).
- **What is half-built:** OAuth device-flow auth ([auth.md](auth.md)), four separate UI
  experiments ([ui.md](ui.md)), logging ([logging-and-errors.md](logging-and-errors.md)).
- **Nothing has tests.** There is no test suite and no CI. Verification means building with
  VitaSDK and running on hardware or in Vita3K ([build-and-release.md](build-and-release.md)).

## Index

| Doc | Covers |
|---|---|
| [architecture.md](architecture.md) | Repo layout, module map, dependency graph, live path vs prototype |
| [build-and-release.md](build-and-release.md) | Toolchain, CMake, linked libraries, VPK contents, versioning, emulator |
| [auth.md](auth.md) | Service-account JWT path, OAuth device flow, on-device secret files, scopes |
| [networking.md](networking.md) | `httpnet` (SceHttp) and curl, init/teardown ordering, response handling |
| [drive-sync.md](drive-sync.md) | Drive API calls, download/upload, change detection, time conversion |
| [saves-and-games.md](saves-and-games.md) | Where saves live on the Vita, game listing, `descript.ion` title database |
| [ui.md](ui.md) | Debug-screen console, text menus, vita2d touch file explorer, ImGui, dialogs, browser |
| [web-companion.md](web-companion.md) | The GitHub Pages verification page and the HTML/JS prototypes |
| [logging-and-errors.md](logging-and-errors.md) | `logger`, the `printf` redefinition, error codes, exit-on-error behavior |
| [conventions.md](conventions.md) | Memory ownership, C/C++ interop, formatting, adding files, platform gotchas |
| [known-issues.md](known-issues.md) | Issues not in `CODE_REVIEW.md`, doc/code mismatches, open questions |

## Reading order for a new task

1. This page, then [architecture.md](architecture.md).
2. The doc for the area you are touching.
3. [conventions.md](conventions.md) before writing code.
4. [known-issues.md](known-issues.md) and `CODE_REVIEW.md` for the files you will edit.

## Keeping these docs true

These docs are only useful while they match the code. When a change alters behavior
described here (a function starts returning a status, a path is uncommented, an auth path
is removed), update the relevant doc in the same commit. Follow `.editorconfig`: 2-space
indent, LF, UTF-8, **no final newline**.