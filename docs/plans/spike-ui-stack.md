# Spike — UI Stack Benchmark

**Question:** which HTML/CSS-style stack can deliver a UI "never seen on the Vita" within
the console's limits, with buttons, front touch and rear touch?

**Needs:** a real Vita (performance, memory and input cannot be judged in Vita3K).

## Candidates

| # | Stack | Kind | Notes to confirm |
|---|---|---|---|
| A | **RmlUi** | Renderer (RML/RCSS, HTML/CSS-like) | C++; data bindings, animations, transitions, flexbox. Needs a render interface (vita2d or GXM) and a font engine (FreeType). **Verify:** existing Vita ports or backends, license (MIT expected), CSS coverage. |
| B | **litehtml** | Layout engine (real HTML/CSS) | C++; layout only, no scripting, limited interactivity and no animations. Needs a "container" that draws, loads fonts and images. **Verify:** license (BSD expected), how events/focus would be handled. |
| C | **JS engine + React-style library** | Logic layer + renderer | A JS engine (e.g. QuickJS, Duktape) running a React-like reconciler (e.g. Preact) that drives A or B, or a custom renderer. Heavy and experimental. **Verify:** engine footprint on ARMv7, existing reconcilers for RmlUi. |
| D | **Web framework compiled to static files** | Authoring layer | Not a renderer: produces HTML/CSS that A or B renders. Evaluate after A/B, as a developer-workflow question on top of the winner. |

## Reference screen

Build **the same screen in each of A, B and C**: a port of the existing browser prototype
`file_explorer.html/.css/.js` (tile grid, header bar, icons via Material Symbols, selection
state, back navigation). It already exists, so the comparison is fair and the winner's
version is reusable.

Required behaviors in the reference screen:

- 60+ items in a scrollable grid with momentum scrolling
- a transition (e.g. header slide-in when selection starts)
- shadows, rounded corners, gradients
- one string in Japanese (CJK font)
- focus navigation with the D-pad, ✕ to open, ○ to go back
- front touch: tap, long-press to select
- rear touch: scroll the grid

## Criteria

Measured or assessed per stack, recorded in a results table:

| Criterion | How |
|---|---|
| Binary size | VPK / eboot size delta vs. an empty app |
| RAM | Peak heap during the reference screen. **Verify** the memory available to a Vita user app and set the budget from it |
| Frame rate | FPS while scrolling and during the transition; frame-time spikes |
| Startup time | Launch to first frame |
| CSS coverage | Checklist: flexbox, grid, transitions, transforms, shadows, gradients, border-radius, custom fonts |
| CJK text | Correct rendering and line breaking |
| Input | D-pad focus model, front touch gestures, rear touch; effort to build each |
| Backend effort | Work to write or adapt a vita2d/GXM render interface |
| Build integration | Effort to build with VitaSDK CMake; dependencies |
| License | Compatibility with the project license (`copyleft-next-0.3.1`) |
| Upstream health | Release activity, issues, community |
| Authoring experience | How close to real HTML/CSS; hot-reload or fast iteration possible? |

## Decision rule (fixed before results)

1. **Hard gates.** A stack is out if it fails any of:
   - sustains at least 30 FPS while scrolling the reference screen (target 60)
   - peak RAM fits within the budget set from the Vita app limit (with room for sync
     buffers)
   - renders CJK text
   - supports D-pad focus and both touch panels without rewriting the engine
   - license compatible with the project
2. **Then** score the remaining stacks on: visual capability (CSS coverage, animations)
   ×3, performance ×2, authoring experience ×2, backend/build effort ×1, upstream health ×1.
3. If C passes the gates but scores within 10% of A or B, prefer the simpler A or B.
4. Evaluate D on top of the winner: does an existing framework output something the
   winner renders well enough to be worth the extra build step?

## Pros / cons

Fill in from the results. Start each list with the expected points and correct them:

### A — RmlUi
- Pros (expected): built for game UIs; data binding; animations; HTML/CSS-like.
- Cons (expected): RCSS is not full CSS; needs a render backend; C++.

### B — litehtml
- Pros (expected): real HTML/CSS; small.
- Cons (expected): no animations or scripting; interactivity has to be built around it.

### C — JS + React-style
- Pros (expected): web-like development; component model.
- Cons (expected): memory and CPU cost on a 2011 console; most experimental.

### D — compiled framework
- Pros (expected): author with familiar web tooling.
- Cons (expected): output limited by the renderer's CSS support; extra build step.

## Deliverable

This file's **Results** section: the measurements table, filled pros/cons, the decision,
and the reasons. Then update [../vision.md](../vision.md#ui) and
[../ui.md](../ui.md).

## Results

_Not started._