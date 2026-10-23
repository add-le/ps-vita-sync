# Logging & Errors

## Logger (`logger.c/.h`)

| Function | Behavior |
|---|---|
| `logger_init()` | `psvDebugScreenInit()`; sets `logger_fd = -1` (file logging TODO) |
| `logger_printf(fmt, …)` | `vsnprintf` into a 4 KB stack buffer, `psvDebugScreenPuts` |
| `logger_panic(code)` | Prints a human-readable message for an SCE error code. Only `SCE_ERROR_ERRNO_ENOENT` is mapped; everything else prints "Unknown error" |
| `logger_exit(status)` | `sceKernelExitProcess(status)` — terminates the app |
| `logger_debug/trace/info/warn/error` | **Declared only.** Not implemented, so calling them fails at link time |

`logger.h` contains `#define printf psvDebugScreenPrintf`. Any file including it (directly
or through another header) prints to the debug screen. `httpnet.c` and `timehandler.c`
repeat the same `#define`. There is no stdout/stderr on the device; `fprintf(stderr, …)` in
`savefile.c` goes nowhere visible.

Nothing is written to a file. Messages disappear when the app exits, which makes field
debugging hard. File logging is a ROADMAP item; a natural location is
`ux0:/data/SYNC00055/`.

## Error codes (`psp2error.h`)

A ~770-line table of `#define SCE_ERROR_* 0x8xxxxxxx` constants (kernel, errno-style, IO,
…) used by `logger_panic`. SCE functions return negative `int`s; print them with `0x%x`
or `0x%X` to match the table.

## Error-handling style today

Three patterns are mixed:

1. **Exit the app**: `logger_printf` + `logger_panic` + `logger_exit(1)`. Used by
   `httpnet.c` (every failure, including any non-200 HTTP status), `appconfig.c`,
   `pkey.c`, `oauth.cpp`, `gdrive.cpp`, `fs.cpp`, `savedata.c`, `appgame.c`.
2. **Return a code**: negative SCE error, `-1`, `NULL`, or a `CURLcode`
   (`savefile.c`, `jwt64.c`, `rs256.c`, `timehandler.c`).
3. **Ignore**: return values unchecked (`RSA_sign`, `fscanf`, `sceHttpReadData`,
   `sceIoClose`).

Placeholder messages like `"todo"`, `"todo 2"`, `"todo 3"` mark unfinished error paths in
`gdrive.cpp` and `oauth.cpp`.

Implications for new code:

- A sync tool should report a failed file and continue with the next one, not quit. Code
  that may fail during sync should return a status and let the caller decide; reserve
  `logger_exit` for unrecoverable startup failures.
- On the success path the app also exits through `logger_exit(0)` after a 3 s delay.
- The ROADMAP plans human-readable errors through native dialogs; see the `dialog.c` notes
  in [ui.md](ui.md#native-helpers).