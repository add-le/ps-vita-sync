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
| `c_quickjs/` | C: QuickJS + Preact 11 (+ htm) rendering into RmlUi through a small DOM shim (`dom.js`), on A's stylesheet and renderer |
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

## Run on the Vita

The hardware rows of the spike (frame rate, RAM, startup) come from this run.

1. Install the five VPKs with VitaShell: `bench_empty.vpk`, `bench_a_rmlui.vpk`,
   `bench_a_rmlui_gl3.vpk`, `bench_b_litehtml.vpk`, `bench_c_quickjs.vpk`. They install as
   separate bubbles ("UI bench …", title IDs `SYNCB0000`, `SYNCB000A`, `SYNCB000G`,
   `SYNCB000B`, `SYNCB000C`) and do not touch the main app.
2. `bench_a_rmlui_gl3` only: it compiles shaders on the console and needs
   `ur0:data/libshacccg.suprx` (extracted from your own firmware with the usual homebrew
   tool for it). Without that file, or if RmlUi's shaders fail to compile through vitaGL,
   expect a black screen or an immediate return to LiveArea. That outcome is a result too:
   note it.
3. Launch each app and do not touch anything for about 20 seconds: the scripted run (idle,
   scroll, transition) plays by itself and the overlay at the bottom right shows the phase.
   When it says `interactive`, try the D-pad, ✕ / ○ / △, tap, hold 500 ms, drag, and the
   rear touch panel, and note how each feels. Press START to quit.
4. Copy `ux0:data/SYNCBENCH/` to a computer. Commit the `.txt` and `.csv` files under
   `benchmarks/ui-stack/results-vita/`, plus a short note per stack on input feel and any
   visual problem.

What each line fills in the spike's Results tables:

| Results file line | Spike row |
|---|---|
| `phase scroll … fps=… interval_ms … work_ms …` | Frame rate while scrolling (gate: ≥ 30 FPS) |
| `phase transition …` | Frame rate during the header transition; for B, the `max` is the document rebuild |
| `heap_bytes … peak=…` minus the `0-empty` peak | Peak RAM (gate: within the 128 MB heap, with room for sync buffers) |
| `free_memory … end[…]` vs. `start[…]` | GPU and other memory outside the heap |
| `startup_ms` | Startup to first frame |
| `input_check …` | Input (should be PASS, as on host) |
| `js_eval_ms`, `js_memory_used_bytes` (C only) | Cost of the JS layer: evaluating Preact and the first render, JS heap |

## Host notes

On host, run with `SDL_RENDER_DRIVER=opengl`: SDL's software renderer drops tiny glyphs (e.g.
`.` at 14 px) and is not representative. Host numbers are only indicative; the Vita numbers
are the ones the decision uses.