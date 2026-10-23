# Spike — Credential Storage

**Question:** how to store OAuth refresh tokens and passwords encrypted with a
device-bound key and an optional PIN, and what that actually protects.

**Needs:** a real Vita for the device-identity part; the crypto design can be done and
unit-tested on the host.

## Requirements (from the vision)

- Tokens and passwords are encrypted at rest under `ux0:/data/SYNC00055/`.
- The key is bound to the console: copying the data folder to another Vita or a PC does
  not reveal secrets.
- Optional PIN to open the app or reveal account settings.
- Moving to a new console means signing in again (acceptable).

## Threat model

| Threat | Protected? |
|---|---|
| Memory card lost, stolen or copied | Yes (key is not on the card) |
| Data folder shared by mistake (e.g. in a bug report) | Yes |
| Other homebrew on the same Vita reading the file | **No**, unless the PIN is part of the key derivation; any app on the console can likely derive the same device key |
| Someone with physical access to the unlocked Vita | Only with the PIN, and only if the PIN is part of the key |

Write this table into the user-facing docs so nobody overstates the protection.

## What to find out

1. **Device identity source.** **Verify** which stable per-console values a user app can
   read (e.g. OpenPSID / console ID APIs) and whether they survive firmware/HENkaku
   changes.
2. **Key derivation.** Device value (+ PIN when set) → KDF (e.g. HKDF, or PBKDF2/scrypt
   when a PIN is involved) → AES-256-GCM key, with OpenSSL (already linked). Store salt,
   KDF parameters and a version byte in the file header for future migration.
3. **PIN behavior.** PIN as an app lock only vs. PIN mixed into the key. Recommendation:
   offer "PIN protects credentials" (mixed into the key; forgetting it means signing in
   again) and record the choice.
4. **Format.** One encrypted file per account or one store; atomic write; corruption
   handling (fall back to "sign in again", never crash).

## Deliverables

- Design note in **Results**.
- A host-tested `core` module (encrypt/decrypt, versioned format) in Phase 2.

## Results

_Not started._