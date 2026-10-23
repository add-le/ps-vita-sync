# Saves & Games

## Where saves live on the Vita

| Kind | Location | Format | Used in code |
|---|---|---|---|
| PSP games (Adrenaline / PSP emulator) | `ux0:/pspemu/PSP/SAVEDATA/<GAMEID…>/` | Plain files (`*.BIN`, `PARAM.SFO`, icons) | Prototype sync (`ULUS10391`, `MHP2NDG.BIN`) |
| Native Vita games | `ux0:/user/00/savedata/<TITLEID>/` | Believed to be encrypted (PFS) when read as raw files | `displaySavedataFolder` lists these folders |
| App data / secrets | `ux0:/data/SYNC00055/` | Plain files | `config.lst`, `pkey.pem`, `config.json` |
| App package (read-only) | `app0:/` | VPK contents | Fonts, `descript.ion` |

**Open question:** native Vita save folders are commonly said to be encrypted per game and
not usable as raw copies without mounting/decrypting them (save managers such as Vita
Save Manager or Apollo do extra work for this). This is unverified for this project. If it
holds, a first sync version should target PSP/Adrenaline saves only, and native-save
support becomes its own feature. Check on hardware before planning native-save sync.

The `UNSAFE` SELF flag (see [build-and-release.md](build-and-release.md)) is what allows
reading/writing outside the app's own sandbox, e.g. `ux0:/pspemu/`.

## Game listing (`savedata.c`)

`displaySavedataFolder(path)` is a text-mode game picker on the debug screen:

1. `sceIoDopen(path)` and read every entry.
2. For each entry, look up a display name with `findNameById("descript.ion", d_name)`;
   entries without a name are reported as "ID … not found" and skipped.
3. Show the list; D-pad Up/Down moves, ✕ selects, ○ deselects, △ confirms.
4. On △ it breaks out of the loop. **The selection is discarded** (`TODO: Returns selected
   games`) and the function returns `void`.

Fixed arrays of 256 entries with no bounds check (CODE_REVIEW #5). Input is polled with
`sceCtrlPeekBufferPositive` plus a 200 ms delay as debounce.

## Game names (`appgame.c`, `descript.ion`)

`descript.ion` is a ~4,467-line text file packaged into the VPK. Format, one per line:

```
<TITLEID> ^ <Game name>
```

It includes PSP, Vita and homebrew IDs (header comments credit NPS Browser and CBPS DB
sources, dated 2022-07-19). `findNameById(filename, id)` opens the file with `fopen`
using a **relative** path and scans it line by line, returning a `strdup`'d name or NULL.
It relies on the working directory resolving to `app0:`. It reopens and rescans the whole
file for every save folder (O(folders × lines)); loading it once would be faster.

## Filesystem helpers (`fs.cpp`)

Used by the touch file explorer (see [ui.md](ui.md)):

- `ls(path)` → `std::vector<File_t*>` with `filename[256]` and `isFolder`
  (`st_attr == SCE_SO_IFDIR`). The `File_t`s are never freed.
- `folder(path)` → `std::vector<Path*>` built from `ls`.
- `Path` is a lazy tree node: `filename`, computed `filepath` (`parent/child/`), children,
  parent, folder and selection flags, `selectAll` / `deselectAll`. The destructor deletes
  children recursively.

Nothing yet turns a set of selected `Path`s into a sync list, and there is no persisted
"what to sync" configuration.