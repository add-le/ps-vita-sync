# UI stack benchmark

Code for the spike in [docs/plans/spike-ui-stack.md](../../docs/plans/spike-ui-stack.md): the
same reference screen (a port of `file_explorer.html`) built on each candidate stack, run
through one harness that measures it the same way. Results and the decision are in the spike
doc; this page is how to build and run.

| Folder | Stack |
|---|---|
| `empty/` | Baseline: the harness with nothing drawn. Size and memory deltas are measured against it |
| `a_rmlui/` | A: RmlUi 6.3, SDL_Renderer backend (patched for the Vita GXM renderer) |
| `b_litehtml/` | B: litehtml (layout) + an SDL_Renderer/SDL_ttf container written here |
| `c_quickjs/` | C: QuickJS + Preact rendering into RmlUi |
| `common/` | Harness, shared data, shared explorer behavior |
| `patches/` | Local patches applied by `fetch.sh` |

## Build

```bash
./fetch.sh                      # engines + Noto Sans JP at pinned commits -> third_party/
cmake -S . -B build-host && cmake --build build-host -j4      # host (Linux, SDL2 dev packages)
./build-vita.sh                 # Vita VPKs via the vitasdk/vitasdk docker image -> build-vita/
```

## The scripted run

Every app runs the same script, then stays interactive until START (Esc on host):

| Time | Phase | Workload |
|---|---|---|
| 0–2 s | `idle` | Nothing |
| 2–12 s | `scroll` | Grid scrolled along a triangle wave, 800 px amplitude, 2 s period |
| 12–18 s | `transition` | Item 1 toggled once per second, so the header slides in and out |
| then | `interactive` | Your input: D-pad + ✕ ○ △, front touch (tap, hold 500 ms to select, drag), rear touch (scroll) |

The overlay (bottom right, SELECT toggles it) shows FPS, frame work time, heap and phase.

Results are written to `ux0:data/SYNCBENCH/<stack>.txt` and `.csv` on the Vita, and to
`results-host/` on host (plus screenshots). Each `.txt` has startup time, stack init time,
heap (start, before the stack, after init, peak), free memory (Vita), and per phase: frame
count, FPS, frame interval p50/p95/p99/max and frame work time (update + render, before
present) p50/p95/p99/max.

On host, run with `SDL_RENDER_DRIVER=opengl`: SDL's software renderer drops tiny glyphs (e.g.
`.` at 14 px) and is not representative. Host numbers are only indicative; the Vita numbers
are the ones the decision uses.