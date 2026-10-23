# Vision

Decision record for what PS Vita Sync should be when finished. Each section gives the
decision, the reason, and what it rules out. Decisions were made by the maintainer in a
structured Q&A on 2026-10-23. Anything marked **Verify** depends on an external fact not
yet checked against an official source.

Plans that implement this vision live in [plans/](plans/README.md).

## Product

**A public, multi-cloud sync client for the PS Vita, with save-game features no generic
cloud client has, and a UI that looks like nothing seen on the Vita before.**

### Audience

Public homebrew release. Setup must be simple enough for anyone with a hacked Vita: no
Google Cloud console, no service accounts, no hand-edited config files.

Rules out: the service-account JWT path as a user-facing auth method (see
[auth.md](auth.md)); `config.lst` / `pkey.pem` / `config.json` as the setup flow.

### Cloud providers

Google Drive, Dropbox, OneDrive, WebDAV / self-hosted (Nextcloud, ownCloud, NAS), Mega.nz,
and the rest of the most-used consumer drives (target: about ten). All go through one
provider interface, so adding a provider does not touch the sync engine or UI.
The candidate list and per-provider capabilities are in
[plans/providers.md](plans/providers.md).

### Accounts

Several accounts can be connected at once (including two accounts of the same provider).
**Each synced item belongs to exactly one account.**

Rules out: syncing one item to several accounts, and primary + mirror setups.

### What syncs

Any file or folder on the Vita, with sync switched on or off per item. On top of that,
saves are first-class (see [Vita-specific features](#vita-specific-features)).

### Vita-specific features

All of these are in scope:

- **Game-aware saves:** saves appear as games (name and icon from `PARAM.SFO` /
  `descript.ion`), not raw title IDs; one tap syncs a game's save.
- **Auto-sync on launch.**
- **Safety net:** a backup before every overwrite, version history, restore an older
  version, and a conflict screen showing both sides (timestamps, sizes).
- **Input:** fully usable with physical buttons, the front touch screen and the **rear
  touch panel**.

### Native Vita saves

Fully supported in the finished app. They are believed to be encrypted per game
(**Verify**), so this starts with a hardware research spike
([plans/spike-native-saves.md](plans/spike-native-saves.md)). PSP/Adrenaline saves and plain
files are not blocked by it.

### Sync model

**Two-way sync; when both sides changed since the last sync, ask the user.**
This needs a persisted record of each item's last synced state (hash, size, remote
revision), which the current code does not have.

Rules out: "newest wins" as the conflict rule, and per-item one-way modes as the default
(they may come back later as an advanced option).

### Cloud layout

**Mirrored by default, remappable.** By default everything lives under one app folder
with the Vita path mirrored, e.g. `/PS Vita Sync/ux0/pspemu/PSP/SAVEDATA/ULUS10391/`, so a
second Vita finds the same files. An advanced option remaps one item to any remote folder.

Known tension: limited scopes (Google `drive.file`, Dropbox App Folder) suit the default
layout but may prevent remapping to arbitrary folders without broader permissions or a
picker. **Verify** per provider; remapping may end up unavailable on some providers.

### Sync triggers

Every trigger is a user setting that can be turned on or off:

- on app launch
- manual (per item and "sync all")
- every N minutes while the app is open
- LiveArea quick-sync (from the app bubble)
- when a game closes, via a companion plugin (feasibility:
  [plans/spike-game-close-plugin.md](plans/spike-game-close-plugin.md))

### History & backups

**Both local and cloud, configurable.** A local backup is always taken on the Vita before
any overwrite. Older versions are also kept in the cloud (provider-native versioning where
available, else a hidden history folder). Users set how many versions each side keeps.

### Sign-in

**No server.** Device-code flow where the provider supports it; otherwise an OAuth flow
whose redirect lands on the project's GitHub Pages site, which shows a code to type on the
Vita; username/password (or app password) for WebDAV and Mega.

Constraint: a public VPK cannot keep a `client_secret` secret. Every OAuth provider must
work as a public client (device flow, or PKCE without a secret). **Verify** per provider.

Rules out: a relay backend, and "bring your own API keys".

### Credential storage

**Encrypted with a device-bound key, plus an optional PIN lock.** Copying `ux0:/data` to
another Vita or a PC does not expose tokens. Moving to a new console means signing in
again.

Threat model, stated honestly: this protects against a copied or lost memory card. It
does not protect against other homebrew running on the same Vita, which can most likely
derive the same device key. The PIN raises that bar only if it is part of the key
derivation. Details: [plans/spike-credential-storage.md](plans/spike-credential-storage.md).

### UI

**HTML/CSS-style UI**, aiming for a look never seen on the Vita. The stack is **not
chosen**; candidates are benchmarked on hardware with pros/cons first
([plans/spike-ui-stack.md](plans/spike-ui-stack.md)):

- **RmlUi** — HTML/CSS-like documents (RML/RCSS), data binding, animations; needs a
  rendering backend.
- **litehtml** — real HTML/CSS layout engine, static documents only; needs a drawing and
  font container.
- **JS-engine React-style stack** — a JavaScript engine running a React-like library
  driving a renderer; heavy and experimental.
- **Compile a web framework to static files** — an **authoring layer**, not a renderer:
  its output still has to be rendered by RmlUi or litehtml. Evaluated on top of the
  winning renderer, not benchmarked as a fourth engine.

### Languages

Follow the Vita's system language. English ships first with a translation system
(strings in files, community translations). Japanese requires a CJK-capable font.

### Codebase

**New layered architecture, salvaging parts.** Providers, sync engine, state store and UI
are designed fresh. Code worth keeping (crypto helpers, Drive calls, `descript.ion`,
icons, the HTML explorer prototype) is ported, fixing its known bugs on the way. The four
UI experiments and dead code are retired.

### Testing

- **Unit tests** do not depend on the Vita: the sync engine, providers, state store and
  parsers build and run on a Linux host.
- **End-to-end tests** run against services started with Testcontainers: real servers
  where self-hostable (WebDAV), and contract-faithful HTTP fakes for SaaS APIs (no official
  containers exist for Drive, Dropbox or OneDrive). Optional live smoke tests with real
  accounts stay out of CI.

See [plans/phase-0-foundations.md](plans/phase-0-foundations.md).

### Distribution

GitHub Releases plus VitaDB, with an **in-app updater** that checks for a new release and
installs it.

### Releases

**v1.0 ships only when everything in this vision is done.** Along the way, numbered 0.x
pre-releases go out so each milestone can be tested on real Vitas.