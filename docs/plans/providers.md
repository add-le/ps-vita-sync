# Providers — Capability Matrix

Research to fill **from each provider's official documentation**, with a source link per
cell. No cell should be filled from memory. Empty cells (`?`) are unknown.

## Constraints every provider must meet

- **Public client.** The VPK is public, so it cannot keep a `client_secret` secret. OAuth
  providers must support a flow without one: device code, or authorization code with
  PKCE as a public client.
- **No server** ([../vision.md](../vision.md#sign-in)). Allowed flows: device code;
  redirect to the GitHub Pages site that displays a code to type on the Vita; a provider's
  own "no redirect" flow that shows a code to copy; username/password or app password.
- **Default layout** is an app folder with mirrored Vita paths; remapping to any folder is
  an advanced option that may need broader scopes.

## Candidate list

The vision asks for "the top 10" drives. Proposed list to confirm with the maintainer:

| # | Provider | Notes |
|---|---|---|
| 1 | Google Drive | Partly built (Drive v3) |
| 2 | Dropbox | |
| 3 | OneDrive | Microsoft Graph |
| 4 | WebDAV | Nextcloud, ownCloud, Synology, QNAP, … |
| 5 | Mega.nz | Client-side encryption; official SDK is a large C++ codebase (**Verify** size, license, portability) |
| 6 | pCloud | |
| 7 | Box | |
| 8 | Yandex Disk | |
| 9 | Koofr | Also exposes WebDAV (**Verify**) |
| 10 | S3-compatible (Backblaze B2, Wasabi, MinIO, …) | Self-hosted friendly; testable with a MinIO container |
| — | iCloud Drive, Proton Drive | **Verify** whether any public third-party API exists; likely out of scope if not |

## Matrix

| Provider | Auth flow (public client) | Device flow? | App-folder / limited scope | Remap needs | Change feed / delta | Native versioning | Upload: max size, resumable | Rate limits | Hash available | E2E test approach |
|---|---|---|---|---|---|---|---|---|---|---|
| Google Drive | ? | ? (**Verify** scopes allowed with the device flow) | `drive.file` / `drive.appdata` ? | Full `drive` is likely a restricted scope needing a security assessment (**Verify**) | `changes` API | Revisions ? | Resumable ? | ? | `md5Checksum` ? | Fake |
| Dropbox | PKCE ? / no-redirect code ? | ? | App Folder ? | Full Dropbox access type ? | `list_folder/continue` ? | Revisions ? | Upload sessions ? | ? | `content_hash` ? | Fake |
| OneDrive | ? | ? | `Files.ReadWrite.AppFolder` ? | `Files.ReadWrite` ? | `delta` ? | Versions ? | Upload sessions ? | ? | `sha1/quickXor` ? | Fake |
| WebDAV | Basic/Digest over HTTPS, app passwords | n/a | n/a | n/a | ETag / `getlastmodified` polling ? | Server-dependent (Nextcloud versions) ? | PUT; chunking server-dependent ? | n/a | Server-dependent ? | Real container |
| Mega.nz | Email + password (+ 2FA) ? | n/a | ? | ? | ? | ? | ? | ? | ? | Live smoke only ? |
| pCloud | ? | ? | ? | ? | ? | ? | ? | ? | ? | Fake |
| Box | ? | ? | ? | ? | ? | ? | ? | ? | ? | Fake |
| Yandex Disk | ? | ? | App folder ? | ? | ? | ? | ? | ? | ? | Fake |
| Koofr | ? | ? | ? | ? | ? | ? | ? | ? | ? | WebDAV container ? |
| S3-compatible | Access key + secret (user-provided) | n/a | Bucket / prefix | n/a | Polling / ETag | Bucket versioning ? | Multipart | ? | ETag (MD5 for single-part) ? | MinIO container |

## Open points per provider

- **Google Drive:** under `drive.file`, can the app write into a user-chosen folder
  without a picker UI? If not, "remap" is unavailable on Google with the default scope.
  Records the tension noted in the vision.
- **Dropbox:** confirm whether a public client with PKCE plus the code-display
  (no-redirect) flow avoids needing the GitHub Pages relay.
- **Mega:** decide between porting the official SDK and implementing the needed subset of
  its protocol and encryption; either is a large task.

## Priority order

Proposed for Phase 3, to confirm after filling the matrix: WebDAV → Google Drive →
OneDrive → Dropbox → S3-compatible → Mega → pCloud → Box → Yandex Disk → Koofr.