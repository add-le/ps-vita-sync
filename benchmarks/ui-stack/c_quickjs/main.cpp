// Stack C: QuickJS runs Preact 11 (+ htm, hooks); a JS DOM shim (dom.js) forwards Preact's
// DOM mutations to RmlUi through the native `rml` bridge below. Styling is stack A's RCSS and
// renderer (SDL_Renderer backend), so C's results measure the cost of the JS layer on top of A.
#include <RmlUi/Core.h>
#include <RmlUi/Core/ElementText.h>
#include <RmlUi_Platform_SDL.h>
#include <RmlUi_Renderer_SDL.h>

extern "C" {
#include <quickjs.h>
}

#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "explorer_state.h"
#include "harness.h"
#include "rml_common.h"

namespace {

constexpr size_t JS_STACK_LIMIT = 512 * 1024;  // below the 1 MB main-thread stack (harness.cpp)

std::string g_assets;

// Element table shared with JS: ids index `elements`; nodes not in the tree are owned by `detached`.
struct Bridge : Rml::EventListener {
  Rml::ElementDocument *document = nullptr;
  std::vector<Rml::Element *> elements{nullptr};
  std::unordered_map<int, Rml::ElementPtr> detached;
  std::unordered_map<Rml::Element *, int> ids;
  JSContext *ctx = nullptr;

  int add(Rml::ElementPtr el) {
    int id = (int)elements.size();
    elements.push_back(el.get());
    ids[el.get()] = id;
    detached[id] = std::move(el);
    return id;
  }
  Rml::Element *get(int id) { return id > 0 && id < (int)elements.size() ? elements[id] : nullptr; }

  void ProcessEvent(Rml::Event &event) override {
    auto it = ids.find(event.GetCurrentElement());
    if (it == ids.end()) return;
    JSValue global = JS_GetGlobalObject(ctx);
    JSValue fn = JS_GetPropertyStr(ctx, global, "__dispatch");
    JSValue args[2] = {JS_NewInt32(ctx, it->second), JS_NewString(ctx, event.GetType().c_str())};
    JSValue r = JS_Call(ctx, fn, global, 2, args);
    JS_FreeValue(ctx, r);
    JS_FreeValue(ctx, args[1]);
    JS_FreeValue(ctx, fn);
    JS_FreeValue(ctx, global);
  }
} g_bridge;

int arg_int(JSContext *ctx, JSValueConst v) {
  int32_t i = 0;
  JS_ToInt32(ctx, &i, v);
  return i;
}

std::string arg_str(JSContext *ctx, JSValueConst v) {
  const char *s = JS_ToCString(ctx, v);
  std::string out = s ? s : "";
  JS_FreeCString(ctx, s);
  return out;
}

#define JS_FN(name) JSValue name(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)

JS_FN(js_create) {
  return JS_NewInt32(ctx, g_bridge.add(g_bridge.document->CreateElement(arg_str(ctx, argv[0]))));
}
JS_FN(js_text) {
  return JS_NewInt32(ctx, g_bridge.add(g_bridge.document->CreateTextNode(arg_str(ctx, argv[0]))));
}
JS_FN(js_set_text) {
  if (auto *t = rmlui_dynamic_cast<Rml::ElementText *>(g_bridge.get(arg_int(ctx, argv[0])))) t->SetText(arg_str(ctx, argv[1]));
  return JS_UNDEFINED;
}
JS_FN(js_insert) {
  Rml::Element *parent = g_bridge.get(arg_int(ctx, argv[0]));
  int child_id = arg_int(ctx, argv[1]);
  Rml::Element *child = g_bridge.get(child_id), *before = g_bridge.get(arg_int(ctx, argv[2]));
  if (!parent || !child) return JS_UNDEFINED;
  Rml::ElementPtr owned;
  auto it = g_bridge.detached.find(child_id);
  if (it != g_bridge.detached.end()) {
    owned = std::move(it->second);
    g_bridge.detached.erase(it);
  } else if (child->GetParentNode()) {
    owned = child->GetParentNode()->RemoveChild(child);  // move within the tree
  }
  if (before) parent->InsertBefore(std::move(owned), before);
  else parent->AppendChild(std::move(owned));
  return JS_UNDEFINED;
}
JS_FN(js_remove) {
  Rml::Element *parent = g_bridge.get(arg_int(ctx, argv[0]));
  int child_id = arg_int(ctx, argv[1]);
  Rml::Element *child = g_bridge.get(child_id);
  if (parent && child && child->GetParentNode() == parent) g_bridge.detached[child_id] = parent->RemoveChild(child);
  return JS_UNDEFINED;
}
// Called by a FinalizationRegistry once JS no longer references a node.
JS_FN(js_release) {
  int id = arg_int(ctx, argv[0]);
  auto it = g_bridge.detached.find(id);
  if (it != g_bridge.detached.end()) {
    g_bridge.ids.erase(g_bridge.elements[id]);
    g_bridge.elements[id] = nullptr;
    g_bridge.detached.erase(it);
  }
  return JS_UNDEFINED;
}
JS_FN(js_attr) {
  if (Rml::Element *el = g_bridge.get(arg_int(ctx, argv[0]))) {
    std::string name = arg_str(ctx, argv[1]);
    if (JS_IsNull(argv[2])) el->RemoveAttribute(name);
    else el->SetAttribute(name, arg_str(ctx, argv[2]));
  }
  return JS_UNDEFINED;
}
JS_FN(js_style) {
  if (Rml::Element *el = g_bridge.get(arg_int(ctx, argv[0]))) {
    std::string name = arg_str(ctx, argv[1]);
    if (JS_IsNull(argv[2])) el->RemoveProperty(name);
    else el->SetProperty(name, arg_str(ctx, argv[2]));
  }
  return JS_UNDEFINED;
}
JS_FN(js_listen) {
  if (Rml::Element *el = g_bridge.get(arg_int(ctx, argv[0]))) el->AddEventListener(arg_str(ctx, argv[1]), &g_bridge);
  return JS_UNDEFINED;
}
JS_FN(js_body) {
  int id = (int)g_bridge.elements.size();
  g_bridge.elements.push_back(g_bridge.document);
  g_bridge.ids[g_bridge.document] = id;
  return JS_NewInt32(ctx, id);
}
// bench.items(): the shared item list (common/model.h) as plain objects.
JS_FN(js_items) {
  JSValue arr = JS_NewArray(ctx);
  auto items = bench::make_items();
  for (size_t i = 0; i < items.size(); i++) {
    JSValue o = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, o, "name", JS_NewString(ctx, items[i].name.c_str()));
    JS_SetPropertyStr(ctx, o, "icon", JS_NewString(ctx, items[i].icon));
    JS_SetPropertyStr(ctx, o, "folder", JS_NewBool(ctx, items[i].folder));
    JS_SetPropertyStr(ctx, o, "synced", JS_NewBool(ctx, items[i].synced));
    JS_SetPropertyUint32(ctx, arr, (uint32_t)i, o);
  }
  return arr;
}

// Module names: "preact", "preact/hooks", "htm" map to the npm ES module files.
char *normalize(JSContext *ctx, const char *, const char *name, void *) {
  const char *file = !strcmp(name, "preact") ? "preact.mjs" : !strcmp(name, "preact/hooks") ? "hooks.mjs"
                   : !strcmp(name, "htm") ? "htm.mjs" : name;
  return js_strdup(ctx, file);
}

JSModuleDef *load_module(JSContext *ctx, const char *name, void *) {
  std::vector<Rml::byte> src = rmlbench::read_file(g_assets + name);
  if (src.empty()) {
    JS_ThrowReferenceError(ctx, "could not load module '%s'", name);
    return nullptr;
  }
  std::string code(src.begin(), src.end());  // JS_Eval needs a NUL-terminated buffer
  JSValue fn = JS_Eval(ctx, code.c_str(), code.size(), name, JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY);
  if (JS_IsException(fn)) return nullptr;
  JSModuleDef *m = (JSModuleDef *)JS_VALUE_GET_PTR(fn);
  JS_FreeValue(ctx, fn);
  return m;
}

void log_exception(JSContext *ctx) {
  JSValue e = JS_GetException(ctx);
  JSValue stack = JS_GetPropertyStr(ctx, e, "stack");
  SDL_Log("JS error: %s\n%s", arg_str(ctx, e).c_str(), JS_IsUndefined(stack) ? "" : arg_str(ctx, stack).c_str());
  JS_FreeValue(ctx, stack);
  JS_FreeValue(ctx, e);
}

class QuickJSStack : public bench::Stack {
public:
  const char *name() const override { return "C-quickjs-preact"; }

  bool init(SDL_Renderer *r, int w, int h, const char *assets) override {
    g_assets = assets;
    render_iface = std::make_unique<RenderInterface_SDL>(r);
    system = std::make_unique<SystemInterface_SDL>(SDL_RenderGetWindow(r));
    files = std::make_unique<rmlbench::AssetFiles>(assets);
    Rml::SetRenderInterface(render_iface.get());
    Rml::SetSystemInterface(system.get());
    Rml::SetFileInterface(files.get());
    Rml::Initialise();
    if (!rmlbench::load_fonts(assets)) return false;
    context = Rml::CreateContext("main", Rml::Vector2i(w, h));
    document = context->LoadDocumentFromMemory(
        "<rml><head><link type=\"text/rcss\" href=\"explorer.rcss\"/></head><body/></rml>", "explorer.rml");
    if (!document) return false;
    document->Show();
    g_bridge.document = document;

    double t0 = SDL_GetPerformanceCounter() / (double)SDL_GetPerformanceFrequency();
    rt = JS_NewRuntime();
    JS_SetMaxStackSize(rt, JS_STACK_LIMIT);
    ctx = JS_NewContext(rt);
    g_bridge.ctx = ctx;
    JS_SetModuleLoaderFunc(rt, normalize, load_module, nullptr);
    install_natives();
    // dom.js is a plain script (globals); app.js is the module entry point.
    if (!eval_file("dom.js", JS_EVAL_TYPE_GLOBAL) || !eval_file("app.js", JS_EVAL_TYPE_MODULE)) return false;
    run_jobs();
    eval_ms = (SDL_GetPerformanceCounter() / (double)SDL_GetPerformanceFrequency() - t0) * 1000;

    context->Update();
    grid = document->GetElementById("grid");
    if (!grid) {
      SDL_Log("Preact rendered nothing");
      return false;
    }
    focus_first();
    return true;
  }

  void button(bench::Button b, bool down) override {
    using K = Rml::Input::KeyIdentifier;
    K key = b == bench::Button::Up ? K::KI_UP : b == bench::Button::Down ? K::KI_DOWN
          : b == bench::Button::Left ? K::KI_LEFT : b == bench::Button::Right ? K::KI_RIGHT : K::KI_UNKNOWN;
    if (key != K::KI_UNKNOWN) {
      if (down) context->ProcessKeyDown(key, 0);
      else context->ProcessKeyUp(key, 0);
      return;
    }
    if (!down) return;
    if (b == bench::Button::Cross) call_app("tap", focused_index());
    else if (b == bench::Button::Circle) call_app("back", -1);
    else if (b == bench::Button::Triangle) call_app("toggle", focused_index());
  }

  void front_touch(const bench::Touch &t) override {
    Rml::TouchList touches{Rml::Touch{(Rml::TouchId)t.id, Rml::Vector2f(t.x, t.y)}};
    if (t.kind == bench::Touch::Down) {
      press.start(t.x, t.y, now, tile_index_at(t.x, t.y));
      context->ProcessTouchStart(touches, 0);
    } else if (t.kind == bench::Touch::Move) {
      press.move(t.x, t.y);
      context->ProcessTouchMove(touches, 0);
    } else {
      press.end();
      if (press.consumed()) context->ProcessTouchCancel(touches);
      else context->ProcessTouchEnd(touches, 0);
    }
  }

  void scroll_by(float dy) override {
    if (grid) grid->SetScrollTop(grid->GetScrollTop() + dy);
  }

  void toggle_select(int index) override { call_app("toggle", index); }

  void update(double t) override {
    now = t;
    int target = press.poll(t);
    if (target >= 0) call_app("toggle", target);
    JSValue global = JS_GetGlobalObject(ctx);
    JSValue tick = JS_GetPropertyStr(ctx, global, "__tick");
    JSValue arg = JS_NewFloat64(ctx, t);
    JS_FreeValue(ctx, JS_Call(ctx, tick, global, 1, &arg));
    JS_FreeValue(ctx, tick);
    JS_FreeValue(ctx, global);
    run_jobs();
    context->Update();
  }

  void render() override {
    render_iface->BeginFrame();
    context->Render();
    render_iface->EndFrame();
  }

  int focused_index() override {
    Rml::Element *e = context->GetFocusElement();
    return e ? e->GetAttribute<int>("index", -1) : -1;
  }

  int folder_depth() override { return depth_now(); }

  const char *extra_results() override {
    JSMemoryUsage m;
    JS_ComputeMemoryUsage(rt, &m);
    char buf[256];
    snprintf(buf, sizeof(buf), "js_eval_ms (dom.js + Preact + app, first render): %.1f\njs_memory_used_bytes: %lld\n"
             "rml_elements_created: %zu\nrml_elements_detached_unreleased: %zu\n", eval_ms,
             (long long)m.memory_used_size, g_bridge.elements.size() - 1, g_bridge.detached.size());
    extra = buf;
    return extra.c_str();
  }

  ~QuickJSStack() override {
    JS_FreeContext(ctx);
    JS_FreeRuntime(rt);
    // Removed subtrees still owned by the bridge must go before RmlUi shuts down.
    g_bridge.detached.clear();
    Rml::Shutdown();
  }

private:
  void install_natives() {
    JSValue global = JS_GetGlobalObject(ctx);
    JSValue rml = JS_NewObject(ctx);
    struct { const char *name; JSCFunction *fn; int argc; } fns[] = {
      {"create", js_create, 1}, {"text", js_text, 1}, {"setText", js_set_text, 2}, {"insert", js_insert, 3},
      {"remove", js_remove, 2}, {"release", js_release, 1}, {"attr", js_attr, 3}, {"style", js_style, 3},
      {"listen", js_listen, 2}, {"body", js_body, 0},
    };
    for (auto &f : fns) JS_SetPropertyStr(ctx, rml, f.name, JS_NewCFunction(ctx, f.fn, f.name, f.argc));
    JS_SetPropertyStr(ctx, global, "rml", rml);
    JSValue b = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, b, "items", JS_NewCFunction(ctx, js_items, "items", 0));
    JS_SetPropertyStr(ctx, global, "bench", b);
    JS_FreeValue(ctx, global);
  }

  bool eval_file(const char *name, int flags) {
    std::vector<Rml::byte> src = rmlbench::read_file(g_assets + name);
    std::string code(src.begin(), src.end());
    JSValue v = JS_Eval(ctx, code.c_str(), code.size(), name, flags);
    if (JS_IsException(v)) {
      log_exception(ctx);
      return false;
    }
    // A module evaluates to a promise: it rejects if the module throws.
    if (flags == JS_EVAL_TYPE_MODULE) {
      run_jobs();
      if (JS_PromiseState(ctx, v) == JS_PROMISE_REJECTED) {
        JS_Throw(ctx, JS_PromiseResult(ctx, v));
        log_exception(ctx);
        JS_FreeValue(ctx, v);
        return false;
      }
    }
    JS_FreeValue(ctx, v);
    return true;
  }

  void run_jobs() {
    JSContext *job_ctx;
    int r;
    while ((r = JS_ExecutePendingJob(rt, &job_ctx)) > 0) {}
    if (r < 0) log_exception(job_ctx);
  }

  // app.<method>(arg), then run Preact's queued render. Navigation resets scroll and focus.
  int call_app(const char *method, int arg) {
    int depth_before = depth_now();
    int result = invoke(method, arg);
    run_jobs();
    if (depth_now() != depth_before) {
      context->Update();
      grid->SetScrollTop(0);
      focus_first();
    }
    return result;
  }

  int depth_now() { return invoke("depth", -1); }

  // Calls app.<method>(arg) (no argument when arg < 0); returns its integer result or -1.
  int invoke(const char *method, int arg) {
    JSValue global = JS_GetGlobalObject(ctx);
    JSValue app = JS_GetPropertyStr(ctx, global, "app");
    JSValue fn = JS_GetPropertyStr(ctx, app, method);
    JSValue a = JS_NewInt32(ctx, arg);
    JSValue r = JS_Call(ctx, fn, app, arg >= 0 ? 1 : 0, &a);
    int result = -1;
    if (JS_IsException(r)) log_exception(ctx);
    else if (JS_IsNumber(r)) result = arg_int(ctx, r);
    JS_FreeValue(ctx, r);
    JS_FreeValue(ctx, fn);
    JS_FreeValue(ctx, app);
    JS_FreeValue(ctx, global);
    return result;
  }

  void focus_first() {
    if (grid && grid->GetNumChildren() > 0) grid->GetChild(0)->Focus(true);
  }

  int tile_index_at(float x, float y) {
    for (Rml::Element *e = context->GetElementAtPoint({x, y}); e; e = e->GetParentNode())
      if (e->IsClassSet("tile")) return e->GetAttribute<int>("index", -1);
    return -1;
  }

  std::unique_ptr<RenderInterface_SDL> render_iface;
  std::unique_ptr<SystemInterface_SDL> system;
  std::unique_ptr<rmlbench::AssetFiles> files;
  Rml::Context *context = nullptr;
  Rml::ElementDocument *document = nullptr;
  Rml::Element *grid = nullptr;
  JSRuntime *rt = nullptr;
  JSContext *ctx = nullptr;
  bench::LongPress press;
  double now = 0, eval_ms = 0;
  std::string extra;
};

}  // namespace

bench::Stack *bench::create_stack() { return new QuickJSStack(); }