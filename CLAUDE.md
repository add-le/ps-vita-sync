# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

PS Vita homebrew app (`ps_vita_sync.vpk`, title ID `SYNC00055`) that syncs game save
files between the Vita and Google Drive. Written in **C/C++**, built with **VitaSDK** +
CMake. The work is in-progress: `src/main.cpp`'s real flow is currently mostly
commented-out exploration, and the auth layer is mid-migration (see "Auth is in flux").

## Build & run

Requires the `$VITASDK` environment variable (CMake errors out fatally otherwise).

```bash
# Build the VPK (from repo root)
mkdir -p build && cd build && cmake .. && make    # -> build/ps_vita_sync.vpk

# Launch in the Vita3K emulator: pass the built VPK to the Vita3K binary, e.g.
#   /path/to/Vita3K *.vpk      # run from build/ after the make above

# Bump version: copies package.json "version" into src/appconfig.h (APP_VERSION)
# and CMakeLists.txt (VITA_VERSION) via sed. Keep all three in sync.
npm run release
```

There is no test suite (the root `test` file is empty). Node 18 (`.nvmrc`) is only for
the `tools/release.ts` ts-node script.

Adding a `.c`/`.cpp` source file requires also adding it to the `add_executable(...)`
list in `CMakeLists.txt` — there is no glob.

## Architecture

The sync pipeline is layered C modules (each `foo.c`/`foo.h`) with a thin C++/UI layer
(`foo.cpp`/`foo.hpp`). `main.cpp` wires them together.

**Auth & crypto (the hard part — Google service-account JWT):**
`appconfig.c` reads `iss`/`kid` → `pkey.c` reads the RSA private key → `jwt64.c` builds
and RS256-signs a JWT (`rs256.c`, `sha256.c`, `base64.c`, OpenSSL-backed) → `driveapi.c`
`getOAuth2Token()` exchanges it for an access token.

**Drive sync:** `driveapi.c` (`getFile`, `getModifiedTime`) + `savefile.c`
(`downloadSavefile`/`uploadSavefile`/`getLastModificationTime`). Change detection
compares Drive `modifiedTime` against the local file mtime (see `main.cpp` and
`FLOWCHART.mmd`/`ROADMAP.md`). `savedata.c` enumerates on-device savedata folders;
`timehandler.c` does time conversions; `appgame.c` maps game IDs to names.

**Networking:** `httpnet.c` wraps GET/POST/PATCH. Note the two HTTP backends don't
coexist: `httpNetInit`/`httpNetClose` (SceHttp) is torn down before curl
(`curl_global_init`) is used for uploads — mind init/close ordering when adding requests.

**JSON:** `json.cpp`/`json.hpp` wrap Sony's `sce::Json` (loaded via
`SCE_SYSMODULE_JSON`); the `Allocator` class lives in `gdrive.hpp`.

**UI:** three separate rendering paths exist. `common/debugScreen.c` text console
(the active one — see logger note below); `gui.cpp` SDL2 file explorer
(`guiFileExplorer`); and Dear ImGui + vita2d (`draw.cpp`, `icon.cpp`, `colors.h`,
`symbols.h`, fonts `Kanit-Regular.ttf` + `MaterialSymbols-Regular.ttf`). `dialog.c`
shows native Vita common dialogs; `browser.c` opens URLs.

**Web companion** (`index.html`, `script.js`, `style.css`, `acces_token.min.html`,
`error.html`): the OAuth device-code **verification page** that renders `user_code` +
`verification_url`. Designed to run on the Vita's WebKit engine.
`file_explorer.{html,css,js}` and `verification_page.jpg` are UI prototypes.

## Conventions & gotchas

- **`printf` is redefined.** `logger.h` does `#define printf psvDebugScreenPrintf`, so
  every `printf` writes to the Vita debug screen, not stdout. The `logger_*` level
  helpers (debug/trace/info/warn/error) and file logging are still TODO stubs.
- **Manual memory ownership.** Functions returning `HttpResponse_t`, `AppConfig_t*`,
  `OAuthCredentials_t`, etc. heap-allocate; callers must call the matching
  `freeHttpResponse` / `freeConfig` / `freeOAuthCredentials`.
- **C included from C++** must be wrapped in `extern "C" { ... }` (see top of `main.cpp`,
  `oauth.cpp`).
- **Secrets live on-device, never in the repo**, under `ux0:/data/SYNC00055/`. Setup is
  documented in `README.md`.
- **Auth is in flux.** Two approaches coexist: `appconfig.c` reads `config.lst`
  (tab-separated `iss`/`kid`, service-account JWT — the documented path), while
  `oauth.cpp` reads `config.json` (`client_id`/`client_secret`, OAuth device flow — the
  ROADMAP "Simplify GDrive Auth" direction, currently incomplete: `loadOAuthCredentials`
  has no return statement). Confirm which path applies before editing auth code.
- **Formatting** (`.editorconfig`): 2-space indent, LF line endings, UTF-8, and
  `insert_final_newline = false` (no trailing newline) with trailing whitespace
  preserved.
