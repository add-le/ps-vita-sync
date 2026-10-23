# Networking

Two HTTP stacks are used, and they are not meant to be active at the same time.

| Stack | Code | Used for |
|---|---|---|
| SceHttp (Sony) | `httpnet.c` | Token exchange, Drive metadata, downloads, device-flow requests |
| libcurl | inside `savefile.c:uploadSavefile` | Drive media upload (PATCH) |

## `httpnet` API

```c
void httpNetInit();      // load NET + HTTPS sysmodules; sceNetInit (16 KB pool),
                         // sceNetCtlInit, sceSslInit (512 KB), sceHttpInit (64 KB)
void httpNetClose();     // reverse order, unload sysmodules
HttpResponse_t httpGet(char *url, HttpHeader_t *headers, size_t n);
HttpResponse_t httpPost(char *url);                       // no headers, no body
HttpResponse_t httpPatch(char *url, char *body, HttpHeader_t *headers, size_t n);
void freeHttpResponse(HttpResponse_t r);                  // frees r.buffer

typedef struct { char *key; char *value; } HttpHeader_t;
typedef struct { char *buffer; unsigned long long length; } HttpResponse_t;
```

All three verbs go through `_httpSend(url, method, headers, n, body)`:

1. `sceHttpCreateTemplate("PS Vita Sync", HTTP/1.1, …)`; headers are added to the template.
2. `sceHttpCreateConnectionWithURL`, `sceHttpCreateRequestWithURL`, `sceHttpSendRequest`.
3. Status code read; **only 200 is accepted**.
4. Body read either by Content-Length (single `sceHttpReadData`) or chunked.
5. Returns `{buffer, length}`. Template, connection and request handles are never deleted.

### Behaviors that constrain new work

- **Any non-200 status, and any SceHttp error, calls `logger_exit(1)` and kills the app.**
  Callers never see failures. This blocks:
  - the device-flow poll (Google answers with non-200 errors while waiting),
  - Drive `files.create` (200) vs. resumable/multipart uploads (other codes), 404 on a
    missing file, 401 on an expired token, 403/429 rate limits.

  Making `_httpSend` return the status (and stop exiting) is a prerequisite for most
  roadmap items.
- **`httpPatch` sends `SCE_HTTP_METHOD_PUT`**, not PATCH. Drive's media upload expects
  PATCH, which is likely why upload was moved to curl.
- `httpPost` cannot send a body or headers; the token exchange puts its form parameters in
  the query string.
- The response buffer has **no NUL terminator** but callers `sscanf` it as a string;
  the chunked branch overwrites the buffer instead of appending. See `CODE_REVIEW.md` #2, #3.
- `sceHttpReadData` can return fewer bytes than asked; the Content-Length branch does not
  loop.
- TLS: SceHttp uses the system's SSL module and certificate store. No custom CA handling is
  done.

## curl (upload)

`uploadSavefile` builds a curl easy handle with:

- `PATCH https://www.googleapis.com/upload/drive/v3/files/<fileId>?uploadType=media`
- headers `Content-Type: application/octet-stream`, `Authorization: Bearer …`,
  `Content-Length: …`
- `CURLOPT_SSL_VERIFYPEER = 0`, `CURLOPT_SSL_VERIFYHOST = 0` — **TLS verification off**
  (CODE_REVIEW #4). Re-enabling it needs a CA bundle shipped in the VPK or read from the
  device.
- TLS 1.2, 10 s connect timeout, follow redirects.

The response body is not read; the HTTP code is fetched but not checked. Only the curl
result code decides success.

## Init / teardown ordering

The prototype does:

```
httpNetInit()            SceHttp stack up
… token, metadata, download via SceHttp …
httpNetClose()           SceHttp stack fully down (including sceNetTerm)
curl_global_init(CURL_GLOBAL_ALL)
uploadSavefile(...)      curl; calls curl_global_cleanup() on success only
```

The two stacks were never used together. The prototype ran curl after `sceNetTerm`; why
that works (or whether curl re-initializes networking itself) is not documented in the
code. When adding requests, keep one stack active at a time, or move everything to one
stack. Consolidating on one stack (SceHttp with real PATCH, or curl for everything) would
remove this ordering hazard.