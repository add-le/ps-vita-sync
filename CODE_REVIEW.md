# PS Vita Sync — Code Review

> Whole-project review covering all `src/` modules plus the web companion.
> **Context:** `main.cpp`'s real flow is almost entirely commented out, so most of this
> code is not yet on the live path. It is reviewed as the code being built toward, not
> as shipping code. The findings hold regardless.

## Overall

The module layering is good: each concern (`base64`, `sha256`, `rs256`, `jwt64`, `pkey`,
`httpnet`, `driveapi`, `savefile`, ...) is a clean `.c`/`.h` pair, headers are documented,
and there is a consistent `freeX()` ownership convention. The architecture is sound.

The problem is execution in C: a systemic, repeated set of memory-safety bugs, plus TLS
verification disabled. Almost every issue is one of a handful of mistakes copy-pasted
across files — fix the patterns once and most of the list collapses.

Severity legend: 🔴 Critical · 🟠 High · 🟡 Medium · 🔵 Low

---

## 🔴 Critical — memory corruption & security

### 1. `malloc(strlen(x))` + `strcpy` — off-by-one heap overflow (3 sites)
Allocation is missing the `+ 1` for the NUL terminator, so each `strcpy` writes one byte
past the buffer.
- `driveapi.c:63` — `access_token = malloc(strlen(acc_token))`
- `driveapi.c:117` — `date_string = malloc(strlen(iso_string))`
- `json.cpp:14` — `caser = malloc(strlen(c_string))`

Fix: `malloc(strlen(x) + 1)` at each site.

### 2. `httpnet.c:121-129` — chunked-transfer read corrupts the heap
```c
recv_buffer = realloc(recv_buffer, read);    // sized to the CURRENT chunk only
memcpy(recv_buffer + prev_read, data, read);  // writes at offset prev_read into that buffer
prev_read = read;                             // should accumulate, not overwrite
```
Three bugs: `realloc` should size to the cumulative total (`prev_read + read`), the
`memcpy` overflows whenever `prev_read > 0`, and `prev_read = read` discards earlier
offsets (should be `prev_read += read`). Result: heap overflow plus only the last chunk
survives. `contentLength` also stays `0` in this branch, so `response.length` is wrong.

### 3. `httpnet.c:116` — response buffer not NUL-terminated but parsed as a string
`recv_buffer = malloc(contentLength)` has no `+ 1` and no terminator, yet
`driveapi.c:58` (`sscanf(token.buffer, ...)`) and `getModifiedTime` (`sscanf(json, ...)`)
read it as a C string → out-of-bounds read. Also `sceHttpReadData` may short-read; it is
not looped to fill `contentLength`.

### 4. `savefile.c:160-161` — TLS verification fully disabled in `uploadSavefile`
```c
curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
```
Sends the OAuth bearer token (full `drive` scope) and save data to anything claiming to
be Google. A MITM can steal the token and tamper with uploads. Should be `1L` / `2L`
with a CA bundle.

### 5. `savedata.c:65,71-83` — fixed `games[256]` with no bounds check
The `sceIoDread` loop does `games[i++]` with no `i < 256` guard. A savedata directory
with more than 256 entries overflows the stack arrays `games[256]` / `selected_games[256]`.

### 6. `gdrive.cpp:124-127` — `sprintf` with format specifiers and zero arguments
```c
sprintf(buffer, "...client_id=%s&client_secret=%s&device_code=%s&grant_type=urn%3A...");
```
Three `%s` (plus `%3A`, which `sprintf` also treats as a conversion) with no variadic
arguments → reads garbage off the stack, undefined behavior, likely crash.

---

## 🟠 High — logic bugs & guaranteed leaks

### 7. `oauth.cpp:17-76` — `loadOAuthCredentials()` never returns a value
Declared to return `OAuthCredentials_t` but has no `return` statement → UB if the result
is used. It only logs `client_id`/`client_secret` (never populates the struct), leaks both
`jsonGet` results, and calls `sceSysmoduleUnloadModule(SCE_SYSMODULE_JSON)` twice
(lines 64 & 75). This is the in-flux OAuth path — incomplete.

### 8. `gdrive.cpp:57-70` — rate-limit branch falls through after destroying the parser
On `rate_limit_exceeded` it calls `initializer.terminate()` and unloads the JSON module
but does not `return`, so execution continues into `jsonGet(val, "device_code")` etc.,
using `val` after the parser is destroyed (UB). Also `error_code` leaks on the normal path.

### 9. `savefile.c:19-43` — `getLastModificationTime`
- `lastModificationTime` is **uninitialized**; if `MHP2NDG.BIN` isn't in the dir it is
  passed to `psp2DateTimeToMs` as garbage → bogus sync decision.
- `dir` (malloc'd) is never freed.
- `if (res < 0)` at line 30 is dead code (the `while` already requires `res > 0`), so
  read errors are silently ignored.
- Filename is hardcoded to `MHP2NDG.BIN`.

### 10. `rs256.c:14-32` — leaks + unchecked signing
`bio` and `rsa` are never freed (`BIO_free` / `RSA_free`) — leaked on every call,
including early-return paths. `RSA_sign`'s return value is unchecked, so on failure
`sigret` / `*sign_length` are garbage and used anyway.

### 11. `appconfig.c` — leak + unbounded scan
- `config_file` (line 22) is never freed on the success path.
- `fscanf(fp, "iss\t%s\nkid\t%s\n", iss, kid)` with unbounded `%s` into `char[256]` →
  overflow on a long config value; return value unchecked.
- `fd` is not closed on the early-return error paths.

### 12. `timehandler.c` — UTC vs. local-time mismatch
`ISOStringToMs` parses Drive's UTC `modifiedTime` with `strptime` then `mktime`, which
interprets the fields as **local** time. `psp2DateTimeToMs` does the same with device-local
mtime. The two sides are not on the same clock, so change detection can choose the wrong
direction. `strptime` also ignores the fractional seconds and trailing `Z`.

### 13. Leaks on error / return paths (pervasive)
- `savefile.c:194` — `uploadSavefile` `return res` skips all five trailing `free()`s.
- `jwt64.c` — leaks `h` / `p` / `hp` on the NULL-return paths.
- `savefile.c:67-70` — `downloadSavefile` leaks `drive_file` and leaves `fd` open on the
  `bw != length` path.
- `fs.cpp` — `folder()` / `ls()` never free the `File_t*` they allocate.
- `httpnet.c` — `tpl` / `conn` / `req` handles are never deleted
  (`sceHttpDeleteRequest` / `...DeleteConnection` / `...DeleteTemplate`) → handle
  exhaustion over many requests.

---

## 🟡 Medium

- **`base64.c:45` — single `=` padding not stripped.** The loop starts at `out_len - 2`,
  so a lone trailing `=` survives. It happens to work for `==` (the earlier `\0`
  truncates), but a 1-pad output yields invalid base64url. Start at `out_len - 1`.
- **`appgame.c:21` — unbounded `sscanf("%s ^ %[^\n]", ...)`** into `char[256]` → overflow
  on a long `descript.ion` line. Use `%255s` / `%255[^\n]`.
- **`dialog.c` — `dialogMsg` only calls `sceMsgDialogInit`.** Common dialogs need an
  update/render loop (`sceMsgDialogGetStatus` + `sceCommonDialogUpdate` inside a vita2d
  frame) or nothing displays. `.sdkVersion = NULL` should be a version constant.
- **`httpnet.c` aborts the whole app (`logger_exit(1)`) on any HTTP/network error**
  instead of returning a status — fragile for a sync tool that should degrade gracefully.
- **`gui.cpp:439`** reads `touch` before `sceTouchRead` on the first iteration
  (uninitialized); **`openFolder` reassigns `root` to a child**, so the final
  `delete root` deletes a subtree and leaks ancestors if the user navigated in.

---

## 🔵 Low / style

- **`script.js` — reflected XSS on the hosted page.** `user_code` / `verification_url`
  come from `location.search` and are written via `innerHTML` (and `verification_url`
  into `href`, allowing `javascript:`). Low risk since the app supplies the values, but
  it is a public page. Use `textContent`, validate the URL scheme, and
  `decodeURIComponent` the values.
- **Exact-fit, fragile buffer sizes:** `pkey_location[29]`, `config_location[31]`, and the
  `36` / `116` magic numbers in `getOAuth2Token` all break if `APP_ID` or the JSON
  templates change. Prefer `snprintf` with computed sizes.
- **Dead code:** `init_string` / `writefunc` in `savefile.c`; the `if (br <= 0) {}` empty
  block in `oauth.cpp:34`.
- **Hardcoded `client_id`** in `gdrive.cpp:18` (acceptable for an installed-app OAuth
  client, but worth a comment).

---

## Recommended order of attack

1. Sweep the `malloc(strlen)+strcpy` pattern (#1) and the `httpnet` buffer handling
   (#2, #3) — exploitable corruption, on the path the moment `main.cpp` is uncommented.
2. Re-enable TLS verification (#4).
3. Bound the fixed arrays / scans (#5, #11, and the medium-section `sscanf`s).
4. Then the logic/leak items (#7, #8, #9, #12, #13) as each module is wired into `main.cpp`.

Most of these are mechanical. A good first commit: the memory-safety sweep (#1–#3, #5)
plus the TLS fix (#4), leaving the WIP/logic items for when those paths are activated.
