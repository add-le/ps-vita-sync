# Spike — Native Vita Saves

**Question:** can the app read and write native Vita game saves
(`ux0:/user/00/savedata/<TITLEID>/`) so they can be synced, and how?

**Needs:** a real Vita with a few native games that have saves, ideally from different
regions and including at least one with protected saves.

## What to find out

1. **Raw access.** With the current `UNSAFE` SELF, does `sceIoOpen` on files under
   `ux0:/user/00/savedata/<TITLEID>/` succeed? Are the bytes plaintext or encrypted?
2. **Encryption model.** **Verify** how Vita savedata is protected (PFS, per-game keys,
   `sce_pfs` / `sce_sys` metadata) from community documentation (e.g. the Vita developer
   wiki).
3. **How existing tools do it.** Study open-source save managers and file managers for the
   Vita (for example VitaShell's handling of savedata, Vita Save Manager, Apollo Save Tool
   for Vita if it exists, savemgr-style projects). **Verify** each one's approach:
   - mounting a game's savedata through system calls (e.g. `sceAppMgr*` game data mount
     APIs) so files appear decrypted at a mount point
   - kernel plugins
   - requirements (does the game need to be installed? same account?)
4. **Writing back.** Can a restored or downloaded save be written so the game accepts it?
   Are there signatures or counters (`sce_sys/` files) that must be preserved or updated?
5. **Cross-console.** Does a save from one Vita work on another (same PSN account /
   different account / no account)? This decides whether multi-Vita sync is possible for
   native saves.
6. **Safety.** What happens if the app is interrupted mid-write? Plan the backup and
   restore flow around the answer.

## Method

1. Read the community documentation and source code first; write down each claim with
   its source.
2. Build a tiny test VPK (not the main app) that tries raw reads and any mount approach
   found, logging results to a file.
3. Try export → modify nothing → import on the same console; then on a second console if
   available.

## Decision to record

- Supported approach (or "not possible without X").
- Whether a kernel plugin is required. If so, how it is installed (the vision allows full
  support with research first; record the user-facing cost).
- What the sync engine must treat as the unit of sync for native saves (whole folder,
  including `sce_sys`?).

## Results

_Not started._