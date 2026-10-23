# Known Issues & Open Questions

`CODE_REVIEW.md` (repo root) is the main bug list: memory safety, TLS, leaks, logic bugs,
with severities and a suggested fix order. This page adds what it does not cover, records
where docs disagree with the code, and lists open questions that block design decisions.

## Issues not in CODE_REVIEW.md

| Severity | Where | Issue |
|---|---|---|
| 🔴 Data loss | `savefile.c:downloadSavefile` | Opens with `SCE_O_WRONLY \| SCE_O_CREAT` but no `SCE_O_TRUNC`: a shorter download leaves stale trailing bytes and corrupts the save. |
| 🔴 Data loss | `savefile.c:downloadSavefile` | The `.old` backup step is commented out (`FIXME`), so a download overwrites the save with no copy. |
| 🔴 Data loss | sync design | No sync history: concurrent changes on both sides are resolved by "newest wins" and the other side is silently lost. See [drive-sync.md](drive-sync.md#change-detection-current-rule). |
| 🟠 Secret exposure | `oauth.cpp:loadOAuthCredentials` | Prints `client_secret` to the screen; it is on the live path, so it shows at every launch. |
| 🟠 Blocker | `httpnet.c:_httpSend` | Any non-200 status exits the app, so device-flow polling and most Drive error handling cannot be written yet. |
| 🟠 | `httpnet.c:httpPatch` | Sends `SCE_HTTP_METHOD_PUT`, not PATCH. |
| 🟠 | `savefile.c:uploadSavefile` | HTTP status not checked: 401/404 count as success. |
| 🟡 | `driveapi.c:getFile` | Passing both `fields` and `download=true` builds a URL with two `?`. |
| 🟡 | `driveapi.c` | Token and `modifiedTime` parsed with fixed-format `sscanf`; breaks on any JSON formatting change. `sce::Json` is already available. |
| 🟡 | `gui.cpp` | No scrolling; `running` is never set to false (explorer cannot exit); the cloud-sync button is a TODO. |
| 🟡 | `savedata.c` | The game picker discards the selection on confirm. |
| 🟡 | `display.c` | Sign-in opens the verification page with a placeholder code. |
| 🔵 | `cloud.hpp` | Missing `;` after the class body; not included anywhere, so it does not break the build yet. |
| 🔵 | `logger.h` | `logger_debug/trace/info/warn/error` declared but not defined; calling one fails at link time. |
| 🔵 | `utils.cpp:hasEnding` | Lower-cases both arguments in place (mutates the caller's strings). |
| 🔵 | `appgame.c` | Rescans the 4.4k-line `descript.ion` per save folder; relies on the working directory resolving to `app0:`. |
| 🔵 | build | SDL2/SDL_ttf and ImGui are linked but effectively unused. |

## Docs vs. code

| Doc says | Code does |
|---|---|
| `README.md`: Step 3 "Change the fileId of google drive" | IDs are hardcoded in commented-out code in `main.cpp`; nothing reads them from config |
| `README.md`: `kid` is "an API Key" | Used as the JWT header `kid` (service-account key ID) |
| `ROADMAP.md`: "Use GAPI to simplify auth" | Implementation started as a hand-written OAuth device flow, not a Google client library (none exists for the Vita) |

## Open questions

These need an answer (from the maintainer or a hardware test) before planning the
related work:

1. **Which auth path stays?** JWT service account (works, heavy setup) or OAuth device flow
   (user-friendly, incomplete)? Removing one simplifies `config` handling and docs.
2. **Which Drive scope?** If `drive.file`, the app must create/own its Drive files and
   folder; hardcoded IDs of hand-uploaded files stop working. Confirm against Google's
   docs, including which scopes the device flow allows.
3. **Native Vita saves?** Are `ux0:/user/00/savedata/*` folders readable/writable as raw
   files with this app's permissions, or encrypted? Decides whether v1 covers PSP saves only.
4. **What is the clock reference?** Are SceIo mtimes UTC or local, and what does `mktime`
   assume on the Vita? Needed for any timestamp comparison.
5. **Sync state storage:** sqlite (ROADMAP) or a simpler manifest file (e.g. JSON per synced
   folder with hash, size and last-synced Drive revision)?
6. **Unit of sync:** single files (prototype) or whole save folders (what users think of
   as "a save")?
7. **One HTTP stack or two?** SceHttp with real PATCH, or curl for everything (which needs a
   CA bundle to re-enable TLS verification)?
8. **Which UI is the base?** The vita2d explorer is the most complete candidate.