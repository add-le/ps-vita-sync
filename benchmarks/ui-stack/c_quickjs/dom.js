// Minimal DOM for Preact 11, backed by RmlUi elements through the native `rml` bridge
// (c_quickjs/main.cpp). Only what Preact touches is implemented.

const listening = new Map(); // element id -> Element, for event dispatch
const release = typeof FinalizationRegistry === "function"
  ? new FinalizationRegistry((id) => rml.release(id))
  : null;

class Node {
  constructor(id, nodeType) {
    this._id = id;
    this.nodeType = nodeType;
    this.parentNode = null;
    this.childNodes = [];
  }
  get firstChild() {
    return this.childNodes[0] || null;
  }
  get nextSibling() {
    const siblings = this.parentNode ? this.parentNode.childNodes : null;
    return siblings ? siblings[siblings.indexOf(this) + 1] || null : null;
  }
  insertBefore(child, ref) {
    if (child.parentNode) child.parentNode._detach(child);
    const i = ref ? this.childNodes.indexOf(ref) : -1;
    if (i < 0) this.childNodes.push(child);
    else this.childNodes.splice(i, 0, child);
    child.parentNode = this;
    rml.insert(this._id, child._id, ref ? ref._id : -1);
    return child;
  }
  appendChild(child) {
    return this.insertBefore(child, null);
  }
  removeChild(child) {
    this._detach(child);
    rml.remove(this._id, child._id);
    return child;
  }
  remove() {
    if (this.parentNode) this.parentNode.removeChild(this);
  }
  _detach(child) {
    const i = this.childNodes.indexOf(child);
    if (i >= 0) this.childNodes.splice(i, 1);
    child.parentNode = null;
  }
}

class Text extends Node {
  constructor(data) {
    super(rml.text(String(data)), 3);
    this._data = String(data);
    if (release) release.register(this, this._id);
  }
  get data() {
    return this._data;
  }
  set data(value) {
    this._data = String(value);
    rml.setText(this._id, this._data);
  }
}

// camelCase style properties to RCSS names; `null`/"" removes the property.
function styleProxy(id) {
  const set = (name, value) => rml.style(id, name, value === null || value === "" ? null : String(value));
  const target = {
    setProperty: set,
    removeProperty: (name) => set(name, null),
    set cssText(text) {
      for (const decl of String(text).split(";")) {
        const at = decl.indexOf(":");
        if (at > 0) set(decl.slice(0, at).trim(), decl.slice(at + 1).trim());
      }
    },
  };
  return new Proxy(target, {
    set(obj, key, value) {
      if (key === "cssText") obj.cssText = value;
      else set(key.replace(/[A-Z]/g, (c) => "-" + c.toLowerCase()), value);
      return true;
    },
  });
}

class Element extends Node {
  constructor(tag, id) {
    super(id === undefined ? rml.create(tag) : id, 1);
    this.localName = tag;
    this.namespaceURI = "http://www.w3.org/1999/xhtml";
    this.attributes = [];
    this.style = styleProxy(this._id);
    if (release && id === undefined) release.register(this, this._id);
  }
  setAttribute(name, value) {
    rml.attr(this._id, name, String(value));
  }
  removeAttribute(name) {
    rml.attr(this._id, name, null);
  }
  addEventListener(type, fn) {
    (this._events || (this._events = {}))[type] = fn;
    listening.set(this._id, this);
    rml.listen(this._id, type);
  }
  removeEventListener(type) {
    if (this._events) delete this._events[type];
  }
}
// Declared so Preact lower-cases "onClick" into the "click" event (it tests `"onclick" in dom`).
Element.prototype.onclick = null;

globalThis.__dispatch = (id, type) => {
  const el = listening.get(id);
  const fn = el && el._events && el._events[type];
  if (fn) fn.call(el, { type, target: el, currentTarget: el });
};

globalThis.document = {
  createElement: (tag) => new Element(tag),
  createElementNS: (ns, tag) => new Element(tag),
  createTextNode: (data) => new Text(data),
};
globalThis.body = new Element("body", rml.body());

// Timers and microtasks, driven once per frame by the harness (__tick).
const timers = [];
let now = 0;
globalThis.queueMicrotask = (fn) => Promise.resolve().then(fn);
globalThis.setTimeout = (fn, ms) => {
  const t = { fn, at: now + (ms || 0) / 1000 };
  timers.push(t);
  return t;
};
globalThis.clearTimeout = (t) => {
  const i = timers.indexOf(t);
  if (i >= 0) timers.splice(i, 1);
};
globalThis.requestAnimationFrame = (fn) => setTimeout(() => fn(now * 1000), 0);
globalThis.cancelAnimationFrame = clearTimeout;
globalThis.__tick = (seconds) => {
  now = seconds;
  for (const t of timers.filter((t) => t.at <= now)) {
    clearTimeout(t);
    t.fn();
  }
};