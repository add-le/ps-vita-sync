# Architecture

## Repository layout

```
.
├── CMakeLists.txt          VitaSDK build: sources, libs, VPK packaging (no globbing)
├── src/                    All app code (C modules + thin C++ layer)
├── common/                 debugScreen text console (vendored VitaSDK sample code)
├── sce_sys/                VPK metadata: icon0.png + LiveArea assets
├── descript.ion            Title-ID → game-name database (packaged into the VPK)
├── Kanit-Regular.ttf       UI font (packaged into the VPK)
├── MaterialSymbols-Regular.ttf  Icon font (packaged into the VPK)
├── index.html script.js style.css   OAuth verification page (served on GitHub Pages)
├── acces_token.min.html error.html  Minified variant / "wrong device" page
├── file_explorer.{html,css,js}      Browser prototype of the file explorer UI
├── verification_page.jpg            Screenshot/mockup of the verification page
├── ps_vita_sync.vpk        A prebuilt VPK committed to the repo (may be stale)
├── tools/release.ts        Version-bump script (npm run release)
├── README.md ROADMAP.md FLOWCHART.mmd CODE_REVIEW.md CLAUDE.md
└── docs/                   This documentation
```

`build/` and `node_modules/` are git-ignored.

## Layering

The code is organized as small C modules (`foo.c` + `foo.h`), each owning one concern,
with a thin C++ layer (`foo.cpp` + `foo.hpp`) for things that need C++ (Sony's
`sce::Json`, `std::vector`, vita2d UI). `main.cpp` wires them together.

```
                         main.cpp
                            │
     ┌──────────────┬───────┴────────┬────────────────┬───────────────┐
     │ Auth         │ Drive sync     │ Saves / games  │ UI            │
     │              │                │                │               │
     │ appconfig.c  │ driveapi.c     │ savedata.c     │ display.c     │
     │ pkey.c       │ savefile.c     │ appgame.c      │ gui.cpp       │
     │ jwt64.c      │ timehandler.c  │ fs.cpp         │  draw.cpp     │
     │  base64.c    │                │                │  icon.cpp     │
     │  sha256.c    │                │                │  utils.cpp    │
     │  rs256.c     │                │                │  colors.h     │
     │ oauth.cpp    │                │                │  symbols.h    │
     │ gdrive.cpp   │                │                │ dialog.c      │
     │  json.cpp    │                │                │ browser.c     │
     └──────┬───────┴───────┬────────┴────────────────┴───────────────┘
            │               │
        httpnet.c (SceHttp)   curl (upload only, inside savefile.c)
            │
        logger.c ── common/debugScreen.c       psp2error.h (error-code table)
```

### Module map

| Module | Role | Doc |
|---|---|---|
| `main.cpp` | Entry point; currently mostly commented-out experiments | below |
| `appconfig.c/.h` | Reads `config.lst` (`iss`, `kid`); defines `APP_ID`, `APP_VERSION` | [auth.md](auth.md) |
| `pkey.c/.h` | Reads `pkey.pem` (service-account RSA private key) | [auth.md](auth.md) |
| `jwt64.c/.h` | Builds and signs a JWT (`header.payload.signature`) | [auth.md](auth.md) |
| `base64.c/.h` | base64url encoding via OpenSSL BIO | [auth.md](auth.md) |
| `sha256.c/.h` | SHA-256 via OpenSSL EVP | [auth.md](auth.md) |
| `rs256.c/.h` | RSA PKCS#1 v1.5 signature of a SHA-256 digest | [auth.md](auth.md) |
| `oauth.cpp/.hpp` | Reads `config.json` (`client_id`, `client_secret`) — incomplete | [auth.md](auth.md) |
| `gdrive.cpp/.hpp` | OAuth device-flow steps; also defines the `sce::Json` `Allocator` | [auth.md](auth.md) |
| `json.cpp/.hpp` | `jsonGet()` helper over `sce::Json::Value` | [auth.md](auth.md) |
| `httpnet.c/.h` | SceHttp wrapper: init/close, GET/POST/"PATCH" | [networking.md](networking.md) |
| `driveapi.c/.h` | Token exchange, `files.get`, `modifiedTime` parsing | [drive-sync.md](drive-sync.md) |
| `savefile.c/.h` | Local mtime, download (SceHttp), upload (curl) | [drive-sync.md](drive-sync.md) |
| `timehandler.c/.h` | `SceDateTime` and ISO-8601 → milliseconds | [drive-sync.md](drive-sync.md) |
| `savedata.c/.h` | Text-mode game picker over a savedata folder | [saves-and-games.md](saves-and-games.md) |
| `appgame.c/.h` | Title-ID → name lookup in `descript.ion` | [saves-and-games.md](saves-and-games.md) |
| `fs.cpp/.hpp` | `ls()` and the `Path` tree used by the file explorer | [ui.md](ui.md) |
| `display.c/.h` | Text-mode main menu | [ui.md](ui.md) |
| `gui.cpp/.hpp` | vita2d touch file explorer | [ui.md](ui.md) |
| `draw.cpp/.hpp` | Rounded rectangles for vita2d | [ui.md](ui.md) |
| `icon.cpp/.hpp`, `utils.cpp/.hpp` | File-extension → icon glyph | [ui.md](ui.md) |
| `colors.h`, `symbols.h` | UI palette and Material Symbols code points | [ui.md](ui.md) |
| `dialog.c/.h` | Native message dialog (incomplete) | [ui.md](ui.md) |
| `browser.c/.h` | Opens a URL in the Vita web browser | [ui.md](ui.md) |
| `cloud.hpp` | Abstract `ICloud` interface; unused and does not compile | [known-issues.md](known-issues.md) |
| `logger.c/.h` | Debug-screen logging, `logger_exit`, `logger_panic` | [logging-and-errors.md](logging-and-errors.md) |
| `psp2error.h` | Large table of `SCE_ERROR_*` codes | [logging-and-errors.md](logging-and-errors.md) |

## The live path

What `main()` executes today (everything else in `main.cpp` is commented out):

```
logger_init()                      debug screen on
printf version banner
OpenSSL_add_all_algorithms()
httpNetInit()                      SceNet / SceNetCtl / SceSsl / SceHttp up
loadOAuthCredentials()             reads ux0:/data/SYNC00055/config.json
                                   prints client_id and client_secret, returns garbage
httpNetClose()
printf "Sync correctly done…"; wait 3 s; logger_exit(0)
```

If `config.json` is missing, `loadOAuthCredentials()` calls `logger_exit(1)` and the app
quits immediately.

## The prototype path (commented out in `main.cpp`)

This is the end-to-end flow that worked at some point, kept as comments:

```
access_token = getOAuth2Token()                         service-account JWT → token
json = getFile(<drive id>, "?fields=modifiedTime")      Drive metadata
cloud_ms = getModifiedTime(json)
local_ms = getLastModificationTime("ux0:/pspemu/PSP/SAVEDATA/ULUS10391/")
if cloud_ms >= local_ms:
    downloadSavefile(<id>, ".../MHP2NDG.BIN")           via SceHttp
    downloadSavefile(<id>, ".../PARAM.SFO")
else:
    httpNetClose(); curl_global_init()
    uploadSavefile(".../MHP2NDG.BIN", <id>)             via curl PATCH
```

Drive file IDs, the save folder and the file name are hardcoded. Only `MHP2NDG.BIN`'s
mtime is compared; `PARAM.SFO` is downloaded but never uploaded.

## Other experiments in `main.cpp`

Commented out alongside the prototype: the device-flow calls
(`requestDeviceAndUserCodes` → `handleTheAuthorizationServerResponse` →
`displayUserCode`), `displaySavedataFolder("ux0:/user/00/savedata/")`,
`displayMainMenu()`, a full Dear ImGui demo loop, `guiFileExplorer()` and
`dialogMsg("Bonjour")`. See [ui.md](ui.md) for which UI is the intended direction.

## Intended design (from ROADMAP.md / FLOWCHART.mmd)

Not implemented; recorded here so plans can reference it:

- Drive side: detect changes with the Drive `changes` API, use `files.create` /
  `files.update` / `files.get`.
- Vita side: detect changes with checksums (MD5/SHA-1), optionally tracked in a local
  sqlite database (checksum, size, timestamps, should-sync flags).
- Deletions on either side disable sync for that file rather than propagating.
- Conflicts resolved manually by the user via a native dialog (timestamp and size shown).
- Simplify auth by moving to OAuth ("Use GAPI").
- File logging and human-readable error dialogs.