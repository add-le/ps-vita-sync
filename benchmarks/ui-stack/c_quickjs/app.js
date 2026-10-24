// Stack C: the reference screen as a Preact component, rendered into RmlUi through dom.js.
// Behavior mirrors common/explorer_state.h; styling is stack A's explorer.rcss, unchanged.
import { h, render } from "preact";
import { useState } from "preact/hooks";
import htm from "htm";

const html = htm.bind(h);
const ICON = {
  back: "", check: "", close: "", cloud: "", selectAll: "", done: "",
};

function load(depth) {
  const items = bench.items();
  const entries = depth > 0 ? [{ name: "..", icon: ICON.back, back: true }] : [];
  const count = depth === 0 ? items.length : Math.floor(items.length / 2);
  for (let i = 0; i < count; i++) entries.push({ ...items[(i + depth * 7) % items.length], selected: false });
  return { depth, entries };
}

function App() {
  const [state, setState] = useState(() => load(0));
  const count = state.entries.filter((e) => e.selected).length;

  const toggle = (i) => setState((s) => ({
    ...s,
    entries: s.entries.map((e, j) => (j === i && !e.back ? { ...e, selected: !e.selected } : e)),
  }));
  const selectAll = (on) => setState((s) => ({
    ...s,
    entries: s.entries.map((e) => (e.back ? e : { ...e, selected: on })),
  }));
  const back = () => state.depth > 0 && setState(load(state.depth - 1));
  const tap = (i) => {
    const e = state.entries[i];
    if (!e) return;
    if (count > 0 && !e.back) toggle(i);
    else if (e.back) back();
    else if (e.folder) setState(load(state.depth + 1));
  };
  // Entry points for the harness (scripted run, long-press, buttons).
  globalThis.app = { tap, toggle, back, selectAll, depth: () => state.depth };

  return html`
    <div id="grid">
      ${state.entries.map((e, i) => html`
        <div key=${i} class=${"tile" + (e.selected ? " selected" : "") + (e.folder ? " folder" : "")}
             index=${i} onClick=${() => tap(i)}>
          <span class="icon">${e.selected ? ICON.check : e.icon}${e.synced && html`<span class="sync">${ICON.done}</span>`}</span>
          <span class="label">${e.name}</span>
        </div>`)}
    </div>
    <div id="header" class=${count > 0 ? "visible" : ""}>
      <div class="group">
        <span class="icon button" onClick=${() => selectAll(false)}>${ICON.close}</span>
        <span>${count + (count > 1 ? " items" : " item")}</span>
      </div>
      <div class="group">
        <span class="icon button">${ICON.cloud}</span>
        <span class="icon button" onClick=${() => selectAll(true)}>${ICON.selectAll}</span>
      </div>
    </div>`;
}

render(h(App), body);