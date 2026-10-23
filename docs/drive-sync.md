# Drive Sync

Covers `driveapi.c`, `savefile.c` and `timehandler.c`: talking to the Drive v3 REST API
and deciding which side wins. None of it is on the live path today; it was exercised by
the commented-out prototype in `main.cpp`.

## Drive API calls

| Function | Request | Notes |
|---|---|---|
| `getOAuth2Token()` | `POST oauth2.googleapis.com/token` | JWT bearer grant; see [auth.md](auth.md) |
| `getFile(id, fields, download, token)` | `GET https://www.googleapis.com/drive/v3/files/<id><fields>[?alt=media]` | `fields` must include its own `?`, e.g. `"?fields=modifiedTime"`. Passing both `fields` and `download` produces two `?` in the URL. |
| `getModifiedTime(json)` | none | `sscanf`s `{\n  "modifiedTime": "…"\n}` — depends on Drive's exact pretty-printing |
| `downloadSavefile(id, path, token)` | `getFile(id, NULL, true, token)` | Writes the body to `path` |
| `uploadSavefile(path, id, token)` | `PATCH …/upload/drive/v3/files/<id>?uploadType=media` via curl | Replaces the content of an **existing** Drive file |

Everything works on **existing Drive file IDs**. There is no `files.create`, no
`files.list`, no folder handling and no `changes` API usage. A new save cannot be uploaded,
and a Drive file cannot be found by name.

## Change detection (current rule)

```
cloud_ms = ISOStringToMs(Drive modifiedTime)
local_ms = psp2DateTimeToMs(mtime of MHP2NDG.BIN in the save folder)
cloud_ms >= local_ms  → download   (ties go to the cloud)
cloud_ms <  local_ms  → upload
```

Limitations:

- **No sync history.** Nothing records the last sync time or a hash of what was synced, so
  "both sides changed since last sync" (a conflict) cannot be told apart from "one side
  changed". Newest timestamp wins and the other side's changes are lost.
- **Clock mismatch.** Drive's `modifiedTime` is UTC (`…Z`). `ISOStringToMs` parses it with
  `strptime` and converts with `mktime`, which treats the fields as **local** time and
  drops the milliseconds and `Z`. `psp2DateTimeToMs` also uses `mktime` on the Vita's
  file time. Whether SceIo mtimes are UTC or local, and what time zone newlib's `mktime`
  uses on the Vita, has not been verified. Treat cross-side comparisons as unreliable until
  it is (CODE_REVIEW #12).
- **Upload resets the cloud mtime** to the upload moment, and **download sets the local
  mtime** to the write moment. The next run therefore sees the side just written as newest
  and syncs the file back the other way, so with no changes at all the direction flips on
  every run. Every other run is a download, which goes through the no-truncate, no-backup
  path below: the data-loss risk recurs rather than being occasional. Any history-based
  design must not rely on mtimes alone for this reason.
- Only one file's mtime is checked; the save folder's other files (`PARAM.SFO`, icons) are
  ignored for the decision.
- `getLastModificationTime` returns garbage if the file is absent (CODE_REVIEW #9).

The ROADMAP's intended replacement: checksums on the Vita side, a local metadata database,
the Drive `changes` API, and manual conflict resolution through a dialog.

## Download

`downloadSavefile`:

1. Fetches the whole file into memory (`HttpResponse_t`).
2. `sceIoOpen(path, SCE_O_WRONLY | SCE_O_CREAT, 0777)` — **no `SCE_O_TRUNC`**. If the new
   content is shorter than the existing file, the old trailing bytes remain and the save is
   corrupted.
3. Writes `length` bytes. On a short write it returns `-1` without closing or freeing.
4. The `.old` backup step (rename the existing save first) is **commented out** with a
   `FIXME`. Downloads overwrite saves with no copy kept.

Returns `1` on success, a negative SCE error or `-1` otherwise.

For a save-sync tool, steps 2 and 4 are the highest data-loss risk in the codebase. A safe
download writes to a temporary file, verifies the size (and ideally a checksum), keeps a
backup, then renames into place.

## Upload

`uploadSavefile` reads the whole file into memory, then sends it with curl (see
[networking.md](networking.md#curl-upload)). Returns `1` on success or the `CURLcode` on
failure. The HTTP status is fetched but not checked, so a 401/404 counts as success. On
failure it returns before freeing its buffers.

## Time helpers (`timehandler.c`)

| Function | Input | Output |
|---|---|---|
| `psp2DateTimeToMs(SceDateTime)` | Vita file time | ms since epoch via `mktime`; prints the date to the screen as a side effect |
| `ISOStringToMs(char *)` | `2024-01-01T12:00:00.000Z` | ms since epoch via `strptime` + `mktime` (seconds precision) |

`timehandler.c` defines `_XOPEN_SOURCE` for `strptime`.