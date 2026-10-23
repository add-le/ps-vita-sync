# User Interface

There are **four separate UI attempts**, none connected to the others or to sync, plus two
native-service helpers. Screen is 960×544.

| UI | Code | Rendering | Input | Status |
|---|---|---|---|---|
| Debug console | `common/debugScreen.c`, `logger.c` | Framebuffer text with ANSI escapes | — | **Active**: the only UI on the live path |
| Text main menu | `display.c` (`displayMainMenu`) | Debug console | D-pad + ✕ | Commented out in `main` |
| Text game picker | `savedata.c` (`displaySavedataFolder`) | Debug console | D-pad + ✕ ○ △ | Commented out in `main` |
| Touch file explorer | `gui.cpp` + `draw.cpp`, `icon.cpp`, `fs.cpp` | **vita2d** + fonts | Front touch | Commented out in `main`; most developed |
| ImGui demo | inline in `main.cpp` | `imgui_vita2d` | Touch + gamepad | Commented out; stock demo only |

The debug console and vita2d both draw to the display, so they should not be mixed in the
same frame loop.

## Debug console

`logger_init()` calls `psvDebugScreenInit()`. `logger.h` does
`#define printf psvDebugScreenPrintf`, so every `printf` in a file that includes
`logger.h` writes to the screen. ANSI escapes work (`\e[0;32m` colors, `\033[row;colH`
cursor moves), which is how the text menus redraw in place. `common/` is vendored VitaSDK
sample code; avoid editing it.

## Text main menu (`display.c`)

Items: "1. Start the synchronization", "2. Manage sync games", "3. Sign in Google Drive
Account", "4. Quit". Only Quit (`logger_exit(0)`) and Sign-in are wired; Sign-in opens the
verification page with a **placeholder** `user_code=XXXX-XXXX`. Infinite loop, no return.

## Touch file explorer (`gui.cpp`)

Entry: `int guiFileExplorer()`. It renders with vita2d; SDL appears only in two
`SDL_SetHint` calls that have no effect.

- Loads `app0:/Kanit-Regular.ttf` (text) and `app0:/MaterialSymbols-Regular.ttf` (icons).
- Starts at `ux0:/` (`root = new Path("ux0:/", folder("ux0:/"))`); folders are listed lazily
  when opened.
- Grid of rounded tiles (231×48, 4 per row, 10 px margin, 6 px gap). The constant
  `GRID_ROW` is actually the number of columns. **No scrolling**: entries beyond the screen
  are drawn off-screen and cannot be reached.
- First tile is a back arrow when the folder has a parent.
- Header bar (shown when ≥ 1 item is selected): close (deselect all), "N items",
  select-all, and a **cloud-sync button whose handler is `// TODO`**.
- Gestures (`handleClickEvent`): tap vs. hold decided by press duration (`TS_HOLD` 500 ms,
  timestamps in µs divided by 1000). Tap a folder to open it; hold to start selecting;
  once selecting, taps toggle selection.
- Touch coordinates are compared against `x * 2, y * 2` because the front touch panel
  reports at twice the screen resolution (1920×1088).
- Main loop `while (running)`: nothing ever sets `running = false`, so the explorer never
  returns.
- Memory: click boxes are `malloc`'d each frame and freed at the next layout; navigation
  reassigns the global `root` to a child (CODE_REVIEW Medium: leaks ancestors).

Visual reference: `file_explorer.{html,css,js}` in the repo root is a browser prototype of
the same design, including a "synced" state per path that the C++ version does not have
yet (see [web-companion.md](web-companion.md)).

### Supporting files

| File | Contents |
|---|---|
| `draw.cpp` | `draw_rounded_rectangle` overloads (1–4 radii) from circles + rectangles |
| `icon.cpp` | `getIcon(filename)` → Material Symbols glyph by extension (code, php, css, html, js, image, audio, font, txt, json; default "note") |
| `utils.cpp` | `hasEnding()` case-insensitive suffix check; **lower-cases its arguments in place** |
| `colors.h` | `RGBA8` palette: `WHITE`, `LIGHT_BLACK`, `GRAY`, `ACCENT`, `SELECTED`, … |
| `symbols.h` | Material Symbols code points (`FOLDER`, `CLOUD_SYNC`, `CHECK_CIRCLE`, …) |

## Native helpers

- **`browser.c` `openUrl(url)`** — loads `SCE_SYSMODULE_APPUTIL`, `sceAppUtilInit`, then
  `sceAppUtilLaunchWebBrowser`. Used to show the OAuth verification page. Launching the
  browser suspends the app.
- **`dialog.c` `dialogMsg(msg)`** — fills `SceMsgDialogParam` and calls
  `sceMsgDialogInit` only. Native common dialogs also need a render loop that calls
  `sceCommonDialogUpdate` each frame and polls `sceMsgDialogGetStatus`, normally inside a
  vita2d or GXM frame, so nothing will display as written. `.sdkVersion = NULL` should be
  the SDK version constant. The ROADMAP plans to use dialogs for errors and conflict
  resolution.

## Choosing a direction

**Superseded for new work:** the [vision](vision.md#ui) chooses an HTML/CSS-style UI,
selected by benchmark ([plans/spike-ui-stack.md](plans/spike-ui-stack.md)). The UIs
described on this page are retired; the HTML explorer prototype is the benchmark's
reference screen. Facts about the existing code, kept for reference:

- The vita2d explorer has the most work, a matching HTML prototype, and touch selection.
  It is the natural base for "choose what to sync".
- Native dialogs need a GXM/vita2d frame loop, which the debug console does not provide.
- ImGui is linked but only the stock demo exists; dropping it (and SDL) would shrink the
  link list.