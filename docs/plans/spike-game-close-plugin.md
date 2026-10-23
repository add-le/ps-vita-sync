# Spike — Game-Close Plugin

**Question:** can a companion plugin detect that a game has exited (and is about to
start) so its save gets synced automatically, and what would it cost users to install?

**Needs:** a real Vita with taiHEN/Ensō.

## What to find out

1. **Detection.** **Verify** which hooks a taiHEN plugin can use to observe application
   launch and exit (user plugin loaded into the shell vs kernel plugin), and how existing
   plugins do similar things (e.g. play-time trackers, auto-backup plugins).
2. **What the plugin can do at that moment.**
   - Network inside a plugin context is likely limited. **Verify.**
   - Options, from simplest to hardest:
     - write a "dirty" marker (`ux0:/data/SYNC00055/pending/<TITLEID>`) that the app
       processes at next launch or via LiveArea quick-sync;
     - launch the PS Vita Sync app with an argument to sync one title, then return;
     - sync in the background from the plugin (probably not feasible).
   - "Pull before launch": can the plugin delay a game's start while a newer save is
     downloaded? Likely not; record what is possible.
3. **Installation.** Plugin type (`*KERNEL` / `*main` / per-title in `config.txt`), how
   the app could install it for the user, and the risk if it misbehaves.
4. **Safety.** Interaction with the game's own save writes; never sync a save while the
   game is running.

## Decision to record

- Feasible approach and user-visible behavior ("syncs when you close a game" vs "marks
  saves for the next sync").
- Installation flow and the setting that enables it.

## Results

_Not started._