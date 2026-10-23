# Build & Release

## Toolchain

- **VitaSDK** is required. `CMakeLists.txt` uses `$VITASDK/share/vita.toolchain.cmake`
  and fails with a fatal error if `VITASDK` is not set.
- CMake ≥ 3.2. Flags: C is `-std=gnu11 -Wall -O2 -mtune=cortex-a9 -mfpu=neon`;
  C++ is `-std=c++11 -fno-rtti -fno-exceptions` (no RTTI, no exceptions: do not use
  `dynamic_cast`, `typeid` or `throw`).
- Node 18 (`.nvmrc`) is only for `tools/release.ts`.

The cloud agent container has no VitaSDK, so code changes cannot be compiled there.
State this explicitly when reporting work that was not built.

## Build

```bash
mkdir -p build && cd build && cmake .. && make    # → build/ps_vita_sync.vpk
```

Sources are listed by hand in `add_executable(...)`. **A new `.c`/`.cpp` file must be
added there** or it will not be compiled (headers do not need listing).

## Libraries

`find_package` requires CURL, OpenSSL, freetype, SDL2 and SDL_ttf (all from the VitaSDK
package set). Linked notable libraries:

| Library | Used by |
|---|---|
| `imgui_vita2d`, `vita2d`, `freetype` | ImGui demo (commented out), `gui.cpp`, `draw.cpp` |
| `SDL2`, `SDL2_ttf` (+ png, jpeg, z) | Only `SDL_SetHint` calls in `gui.cpp`; effectively unused |
| `curl`, `OpenSSL`/`crypto` | Upload (`savefile.c`); JWT signing (`base64`/`sha256`/`rs256`) |
| `SceHttp`, `SceSsl`, `SceNet`, `SceNetCtl` | `httpnet.c` |
| `SceLibJson` | `sce::Json` in `gdrive.cpp`, `oauth.cpp`, `json.cpp` |
| `SceCommonDialog`, `SceAppUtil` | `dialog.c`, `browser.c` |
| `SceTouch`, `SceCtrl`, `SceDisplay`, `SceGxm` | Input and rendering |
| `SceKernelUtilsForDriver_stub`, `SceVshBridge_stub` | Linked; `savefile.c` includes a `psp2kern` header |

The SELF is created with `vita_create_self(... UNSAFE)`, which gives the app extended
permissions (needed for some file-system access).

## VPK contents

`vita_create_vpk` packages:

| Source | In VPK (readable at `app0:/…`) |
|---|---|
| `sce_sys/icon0.png` | Bubble icon |
| `sce_sys/livearea/contents/{bg,startup}.png`, `template.xml` | LiveArea screen |
| `descript.ion` | Game-name database |
| `Kanit-Regular.ttf`, `MaterialSymbols-Regular.ttf` | Fonts for `gui.cpp` |

Any new runtime asset must be added as a `FILE src dst` line here.

`VITA_MKSFOEX_FLAGS` sets `PARENTAL_LEVEL=1`. App name in LiveArea: "PS Vita Sync".

## Running

- **Hardware:** install the VPK with VitaShell on a HENkaku/Ensō Vita. The app needs the
  secret files described in [auth.md](auth.md) under `ux0:/data/SYNC00055/`.
- **Emulator:** Vita3K can install the VPK (`/path/to/Vita3K build/ps_vita_sync.vpk`).
  Network, SceHttp/SSL and `sceAppUtilLaunchWebBrowser` support in the emulator is limited,
  so auth and sync paths are best verified on hardware.

## Versioning & release

The version lives in three places that must stay equal:

| File | Field |
|---|---|
| `package.json` | `"version": "00.02"` (source of truth) |
| `src/appconfig.h` | `#define APP_VERSION "00.02"` |
| `CMakeLists.txt` | `set(VITA_VERSION "00.02")` |

`npm run release` runs `tools/release.ts`, which `sed`-replaces the other two from
`package.json`. Format is `XX.YY` (Vita `APP_VER` format). Bump `package.json` first, then
run the script.

## Committed binary

`ps_vita_sync.vpk` at the repo root is a committed build artifact. It is not rebuilt
automatically and may not match the current source.