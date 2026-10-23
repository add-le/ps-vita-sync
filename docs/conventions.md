# Conventions

## Code style

- `.editorconfig`: 2-space indent, LF, UTF-8, `insert_final_newline = false` (files end
  **without** a trailing newline), `trim_trailing_whitespace = false`.
- C code looks clang-format (LLVM style) formatted: 80 columns, `type *name`, braces on
  the same line. Match the surrounding file.
- Names: `camelCase` functions (`getFile`, `downloadSavefile`), `snake_case` locals and
  struct fields, `PascalCase` structs with a `_t` typedef (`HttpResponse_t`,
  `AppConfig_t`). Exceptions: `logger_*`, `base64url_encode`, `draw_rounded_rectangle`.
- Header guards: `#ifndef _NAME_H_` / `_NAME_HPP_`.
- Public functions get a `/** … @param … @return … */` doc comment in the header, with a
  line saying "Result must be freed when not used anymore" when the caller owns the result.
- Commits use Conventional Commits with a scope: `feat(gui): …`, `fix(gui): …`,
  `docs: …`, `chore(ai): …`.

## Memory ownership

Manual and by convention:

| Returned by | Free with |
|---|---|
| `httpGet` / `httpPost` / `httpPatch` / `getFile` → `HttpResponse_t` | `freeHttpResponse` |
| `loadConfig()` → `AppConfig_t *` | `freeConfig` |
| `loadOAuthCredentials()` → `OAuthCredentials_t` | `freeOAuthCredentials` |
| `handleTheAuthorizationServerResponse()` → `OAuth2Response_t` | `freeOAuth2Response` |
| `getOAuth2Token`, `loadPkey`, `newJwt`, `signRs256`, `base64url_encode`, `jsonGet`, `findNameById` → `char *` | `free` |
| `folder()` → `Path *`s | `delete` the root (recursive) |

Recurring mistakes to avoid (see `CODE_REVIEW.md`): `malloc(strlen(s))` without `+ 1`;
early `return`s that skip the frees at the end of a function; treating a non-terminated
`HttpResponse_t.buffer` as a C string; fixed-size buffers filled with unbounded
`sprintf`/`sscanf("%s")`. Prefer `snprintf` with computed sizes, `%255s`-style widths, and a
single cleanup label (`goto cleanup`) in C functions with several allocations.

## C and C++ together

- C headers included from `.cpp` files must be inside `extern "C" { … }` (see the top of
  `main.cpp`, `oauth.cpp`, `gdrive.cpp`, `gui.cpp`).
- `gdrive.hpp` is a C++ header (it uses `sce::Json`) that both `main.cpp` and `gdrive.cpp`
  include **inside** `extern "C"`, so its functions get C linkage on both sides. If one
  side moves the include out of the block, the call sites and definitions stop linking.
  `oauth.hpp` is included outside `extern "C"` everywhere (C++ linkage).
- C++ is built with `-fno-rtti -fno-exceptions`, C++11. No `throw`, `dynamic_cast` or
  `typeid`; avoid STL features that require exceptions to report errors.
- C files use designated initializers and GNU C11.

## Platform gotchas

- `printf` is redefined to the debug screen (see
  [logging-and-errors.md](logging-and-errors.md)).
- Device paths use a device prefix: `ux0:/…` (memory card), `app0:/…` (the installed
  package, read-only), `ur0:`, etc. Paths are not POSIX-rooted.
- Sysmodules (`SCE_SYSMODULE_NET`, `HTTPS`, `JSON`, `APPUTIL`) must be loaded before use and
  are unloaded afterwards in the current code. Load/unload must be balanced
  (`oauth.cpp` unloads JSON twice).
- File I/O uses `sceIo*` (`sceIoOpen` flags `SCE_O_RDONLY`, `SCE_O_WRONLY`, `SCE_O_CREAT`,
  `SCE_O_TRUNC`), returning negative error codes. `sceIoLseek(fd, 0, SCE_SEEK_END)` is the
  file-size idiom used here.
- `sceKernelDelayThread` takes microseconds.
- Input: `sceCtrlPeekBufferPositive` for buttons; `sceTouchRead` for touch, with front-panel
  coordinates at 2× screen resolution.
- Secrets live on the device under `ux0:/data/SYNC00055/`, never in the repo.

## Adding things

- New source file → add it to `add_executable(...)` in `CMakeLists.txt`.
- New runtime asset → add a `FILE src dst` line to `vita_create_vpk(...)`; read it from
  `app0:/dst`.
- New system library → add the `*_stub` to `target_link_libraries`.
- Version bump → edit `package.json`, run `npm run release`.
- Behavior change → update the matching file in `docs/`.

## Verifying changes

There are no tests and no CI. Building requires VitaSDK (not in the cloud container).
State plainly whether a change was compiled and whether it was run on hardware, in
Vita3K, or not at all.