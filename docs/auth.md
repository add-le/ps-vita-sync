# Authentication

Two approaches coexist in the current code. This page describes them as they are.

**Direction for new work:** the [vision](vision.md#sign-in) settles it: public-client
OAuth flows (device code, or a code shown on the GitHub Pages site) with no server, plus
username/password for WebDAV and Mega, credentials encrypted on the device. The
service-account JWT path is not a user-facing option in the finished app. For changes to
the existing code, still confirm which path a task targets.

| | Service-account JWT | OAuth device flow |
|---|---|---|
| Status | Worked in the prototype; documented in `README.md` | Incomplete; the only auth code on the live path |
| Secrets on device | `config.lst` + `pkey.pem` | `config.json` |
| Code | `appconfig.c`, `pkey.c`, `jwt64.c`, `base64.c`, `sha256.c`, `rs256.c`, `driveapi.c:getOAuth2Token` | `oauth.cpp`, `gdrive.cpp`, `json.cpp`, web page |
| Scope requested | `https://www.googleapis.com/auth/drive` (full Drive) | `https://www.googleapis.com/auth/drive.file` |
| User setup | Google Cloud console, service account, share a folder with it | Visit a URL, enter a code |
| Direction | Legacy | ROADMAP "Simplify GDrive Auth" |

## On-device files

All under `ux0:/data/SYNC00055/` (`APP_ID` in `appconfig.h`). **Never commit secrets.**

| File | Format | Read by |
|---|---|---|
| `config.lst` | `iss\t<service-account email>\nkid\t<key id>\n` — exact, tab-separated, LF | `loadConfig()` |
| `pkey.pem` | RSA private key PEM, LF line endings, ends with `-` (no trailing newline) | `loadPkey()` |
| `config.json` | JSON with `client_id`, `client_secret` | `loadOAuthCredentials()` |

`README.md` describes `kid` as an "API Key". In the JWT header it is used as the key ID of
the service-account key; see Google's service-account docs if this needs correcting.

## Path 1 — service-account JWT

`getOAuth2Token()` in `driveapi.c`:

1. `loadConfig()` → `AppConfig_t { iss, kid }`; `loadPkey()` → PEM string.
2. Header `{"alg":"RS256","typ":"JWT","kid":"<kid>"}`.
3. Payload `{"iss":"<iss>","iat":<now>,"exp":<now+3600>,"scope":"https://www.googleapis.com/auth/drive","aud":"https://oauth2.googleapis.com/token"}`.
   `now` comes from `time()`, so the Vita clock must be roughly correct.
4. `newJwt()` (`jwt64.c`): base64url(header) `.` base64url(payload) → SHA-256
   (`hashSha256`) → RSA sign (`signRs256`, `RSA_sign(NID_sha256, …)`) → base64url(sig).
5. `httpPost("https://oauth2.googleapis.com/token?grant_type=urn%3Aietf%3Aparams%3Aoauth%3Agrant-type%3Ajwt-bearer&assertion=<jwt>")`.
   Parameters are sent in the **query string** with an empty body.
6. The response is parsed with a fixed-format `sscanf` expecting
   `{"access_token":"…","expires_in":…,"token_type":"Bearer"}` exactly. Any formatting or
   key-order difference breaks the parse silently.
7. Returns a heap `char *access_token` (caller frees). The token lasts ~1 hour; nothing
   refreshes it.

Crypto helpers use the OpenSSL 1.x API (`RSA_*`, `PEM_read_bio_RSAPrivateKey`,
`EVP_MD_CTX_create`). Leaks and unchecked returns are listed in `CODE_REVIEW.md` (#10, #13)
and the base64 padding bug in its Medium section.

## Path 2 — OAuth 2.0 device flow (incomplete)

Google's flow for limited-input devices. Intended sequence:

```
requestDeviceAndUserCodes()                 POST oauth2.googleapis.com/device/code
        │                                   ?client_id=…&scope=drive.file
        ▼
handleTheAuthorizationServerResponse(json)  parse device_code, user_code,
        │                                   verification_url, expires_in, interval
        ▼
displayUserCode(resp)                       open https://add-le.github.io/ps-vita-sync/
        │                                   ?user_code=…&expires_in=…&verification_url=…
        │                                   in the Vita browser (see web-companion.md)
        ▼
pollAuthorizationServer()                   POST oauth2.googleapis.com/token with
                                            client_id, client_secret, device_code,
                                            grant_type=urn:ietf:params:oauth:grant-type:device_code
                                            every `interval` seconds until approved
```

What exists in code:

- `requestDeviceAndUserCodes()` — implemented. `client_id` is **hardcoded** in
  `gdrive.cpp` rather than read from `config.json`.
- `handleTheAuthorizationServerResponse()` — implemented with `sce::Json`, but on
  `rate_limit_exceeded` it tears down the parser and keeps going (CODE_REVIEW #8).
  `expires_in` and `interval` are kept as strings.
- `displayUserCode()` — implemented; builds the URL with `sprintf` into a 2 KB buffer and
  calls `openUrl()`. Values are not URL-encoded.
- `pollAuthorizationServer()` — **stub**: a `sprintf` with three `%s` and no arguments
  (CODE_REVIEW #6). No loop, no interval handling, no token parsing, no storage.
- `loadOAuthCredentials()` (`oauth.cpp`) — reads `config.json`, but only **prints**
  `client_id` and `client_secret` to the screen; it never fills or returns the struct
  (CODE_REVIEW #7). It is called on the live path, so the secret appears on screen at every
  launch.

Missing for a working device flow:

- `httpnet` must return HTTP status codes instead of exiting. While waiting for the user,
  Google answers the poll with error statuses such as `authorization_pending` and
  `slow_down` (see [networking.md](networking.md)).
- A refresh token must be stored on device (e.g. under `ux0:/data/SYNC00055/`) and used to
  get new access tokens, so the user does not repeat the flow every launch.
- A decision on scope (below).

### Scope consequences

With `drive.file`, as far as Google documents it, the app can only see files it created or
that the user explicitly opened/shared with it. The prototype's Drive file IDs point at
files uploaded by hand, which would likely be invisible under `drive.file`. Moving to the
device flow therefore probably also means the app must create (or let the user pick) its
own folder and files. Google also limits which scopes the device flow accepts. Verify both
points against Google's current docs before planning around them.

## `sce::Json` usage

Sony's JSON library is loaded on demand with `sceSysmoduleLoadModule(SCE_SYSMODULE_JSON)`.
Pattern used in `gdrive.cpp` and `oauth.cpp`:

```cpp
Allocator *a = new Allocator();                 // gdrive.hpp: malloc/free wrapper, never deleted
sce::Json::InitParameter p = {.allocator = a, .bufSize = 256};
sce::Json::Initializer init; init.initialize(&p);
sce::Json::Value v; sce::Json::Parser::parse(v, text, strlen(text));
char *s = jsonGet(v, "key");                    // heap copy, caller frees (has a +1 bug)
init.terminate(); sceSysmoduleUnloadModule(SCE_SYSMODULE_JSON);
```

`jsonGet` converts any value to a string. Its behavior for a missing key has not been
verified.