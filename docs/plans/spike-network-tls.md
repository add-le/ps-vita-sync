# Spike — Network & TLS

**Question:** which HTTP/TLS stack should the app use for all providers, and does it work
with today's provider endpoints from a real Vita?

**Needs:** a real Vita on Wi-Fi.

## Background

Today two stacks coexist ([../networking.md](../networking.md)): SceHttp (system SSL and
CA store) and curl with TLS verification disabled. The Vita's last firmware is old, so its
built-in CA store and TLS versions may not match what current endpoints require.
**Verify** everything below on hardware.

## What to test

For each endpoint, with each stack:

| Endpoint | Provider |
|---|---|
| `oauth2.googleapis.com`, `www.googleapis.com` | Google |
| `api.dropboxapi.com`, `content.dropboxapi.com` | Dropbox |
| `login.microsoftonline.com`, `graph.microsoft.com` | OneDrive |
| `g.api.mega.co.nz` (**Verify** host) | Mega |
| A Let's Encrypt-certified WebDAV server | WebDAV |
| `api.github.com` (updater) | Updater |

Stacks:

1. **SceHttp + system SSL** (as today).
2. **curl + bundled TLS library** (whatever the VitaSDK curl package is built with;
   **Verify** OpenSSL vs mbedTLS) **+ a bundled CA file** (e.g. Mozilla's `cacert.pem`
   packaged in the VPK) with verification **on**.

Record per pair: handshake success, TLS version, certificate verification result,
throughput for a 10 MB download and upload, memory use.

Also check:

- Real `PATCH`, custom headers and request bodies (SceHttp's `httpPatch` currently sends
  PUT).
- Chunked transfer, `Content-Range` / resumable uploads.
- Behavior when Wi-Fi drops mid-transfer; timeouts.
- Whether curl needs the SceNet stack initialized (the prototype ran curl after
  `sceNetTerm`).

## Decision to record

- One stack for everything (preferred), with the reason.
- How the CA bundle is shipped and updated (with app updates).
- Required init/teardown order.

## Results

_Not started._