// Stack A: RmlUi 6.3. Default build: SDL_Renderer backend (Backends/RmlUi_Renderer_SDL.cpp,
// patched for straight alpha). A_GL3 build: the GL3 backend with every effect enabled (A_EFFECTS).
#include <RmlUi/Core.h>
#include <RmlUi_Platform_SDL.h>
#ifdef A_GL3
#include <RmlUi_Renderer_GL3.h>
using RenderInterface = RenderInterface_GL3;
#else
#include <RmlUi_Renderer_SDL.h>
using RenderInterface = RenderInterface_SDL;
#endif

#include <cstdio>
#include <memory>
#include <vector>

#include "explorer_state.h"
#include "harness.h"
#include "rml_common.h"

namespace {

struct ItemVM {
  Rml::String name, icon;
  bool folder = false, synced = false, selected = false;
};

class RmlStack : public bench::Stack {
public:
#ifdef A_GL3
  const char *name() const override { return "A-rmlui-gl3"; }
#else
  const char *name() const override { return "A-rmlui"; }
#endif

  bool init(SDL_Renderer *r, int w, int h, const char *assets) override {
    sdl_renderer = r;
#ifdef A_GL3
    // Runs inside SDL's OpenGL renderer context (SDL_RENDER_DRIVER=opengl).
    Rml::String gl_message;
    if (!RmlGL3::Initialize(&gl_message)) return false;
    SDL_Log("%s", gl_message.c_str());
    render_iface = std::make_unique<RenderInterface_GL3>();
    render_iface->SetViewport(w, h);
#else
    render_iface = std::make_unique<RenderInterface_SDL>(r);
#endif
    system = std::make_unique<SystemInterface_SDL>(SDL_RenderGetWindow(r));
    files = std::make_unique<rmlbench::AssetFiles>(assets);
    Rml::SetRenderInterface(render_iface.get());
    Rml::SetSystemInterface(system.get());
    Rml::SetFileInterface(files.get());
    Rml::Initialise();
    if (!rmlbench::load_fonts(assets)) return false;

    context = Rml::CreateContext("main", Rml::Vector2i(w, h));
    if (!context) return false;

    Rml::DataModelConstructor c = context->CreateDataModel("explorer");
    if (auto s = c.RegisterStruct<ItemVM>()) {
      s.RegisterMember("name", &ItemVM::name);
      s.RegisterMember("icon", &ItemVM::icon);
      s.RegisterMember("folder", &ItemVM::folder);
      s.RegisterMember("synced", &ItemVM::synced);
      s.RegisterMember("selected", &ItemVM::selected);
    }
    c.RegisterArray<std::vector<ItemVM>>();
    c.Bind("items", &items);
    c.Bind("selected_count", &state.selected_count);
    c.BindEventCallback("tap", [this](Rml::DataModelHandle, Rml::Event &, const Rml::VariantList &args) {
      if (args.empty() || press.consumed()) return;
      apply(state.tap(args[0].Get<int>()));
    });
    c.BindEventCallback("select_all", [this](Rml::DataModelHandle, Rml::Event &, const Rml::VariantList &) {
      apply(state.select_all(true));
    });
    c.BindEventCallback("deselect_all", [this](Rml::DataModelHandle, Rml::Event &, const Rml::VariantList &) {
      apply(state.select_all(false));
    });
    model = c.GetModelHandle();

    state.open_root();
    sync_items();
    std::vector<Rml::byte> rml = rmlbench::read_file(std::string(assets) + "explorer.rml");
    Rml::String source(rml.begin(), rml.end());
#ifdef A_EFFECTS
    source = Rml::StringUtilities::Replace(source, "<!--EFFECTS-->", "<link type=\"text/rcss\" href=\"explorer-effects.rcss\"/>");
#endif
    document = context->LoadDocumentFromMemory(source, "explorer.rml");
    if (!document) return false;
    document->Show();
    grid = document->GetElementById("grid");
    context->Update();
    focus_first();
    return true;
  }

  void button(bench::Button b, bool down) override {
    using K = Rml::Input::KeyIdentifier;
    K key = K::KI_UNKNOWN;
    switch (b) {
      case bench::Button::Up: key = K::KI_UP; break;
      case bench::Button::Down: key = K::KI_DOWN; break;
      case bench::Button::Left: key = K::KI_LEFT; break;
      case bench::Button::Right: key = K::KI_RIGHT; break;
      default: break;
    }
    if (key != K::KI_UNKNOWN) {
      if (down) context->ProcessKeyDown(key, 0);
      else context->ProcessKeyUp(key, 0);
      return;
    }
    if (!down) return;
    if (b == bench::Button::Cross) apply(state.tap(focused_tile()));
    else if (b == bench::Button::Circle) apply(state.back());
    else if (b == bench::Button::Triangle) apply(state.toggle(focused_tile()));
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
      // A fired long-press must not also produce a click.
      if (press.consumed()) context->ProcessTouchCancel(touches);
      else context->ProcessTouchEnd(touches, 0);
    }
  }

  void scroll_by(float dy) override {
    if (grid) grid->SetScrollTop(grid->GetScrollTop() + dy);
  }

  void toggle_select(int index) override { apply(state.toggle(index)); }

  void update(double t) override {
    now = t;
    int target = press.poll(t);
    if (target >= 0) apply(state.toggle(target));
    context->Update();
  }

  void render() override {
#ifdef A_GL3
    SDL_RenderFlush(sdl_renderer);  // raw GL calls follow; the backend saves and restores GL state
    // SDL's GL renderer leaves GL_UNPACK_ROW_LENGTH set after its own texture uploads (the overlay),
    // which corrupts RmlUi's glyph atlas uploads. Host-only integration detail.
    static auto pixel_store = (void (*)(unsigned, int))SDL_GL_GetProcAddress("glPixelStorei");
    pixel_store(0x0CF2 /* GL_UNPACK_ROW_LENGTH */, 0);
    pixel_store(0x0CF5 /* GL_UNPACK_ALIGNMENT */, 4);
#endif
    render_iface->BeginFrame();
    context->Render();
    render_iface->EndFrame();
  }

  int focused_index() override { return focused_tile(); }
  int folder_depth() override { return state.depth; }

  ~RmlStack() override { Rml::Shutdown(); }

private:
  void apply(bench::ExplorerState::Result r) {
    if (r == bench::ExplorerState::None) return;
    sync_items();
    model.DirtyVariable("items");
    model.DirtyVariable("selected_count");
    if (r == bench::ExplorerState::Navigated) {
      context->Update();
      grid->SetScrollTop(0);
      focus_first();
    }
  }

  void sync_items() {
    items.resize(state.entries.size());
    for (size_t i = 0; i < items.size(); i++) {
      const auto &e = state.entries[i];
      items[i] = {e.name, e.shown_icon(), e.folder, e.synced, e.selected};
    }
  }

  void focus_first() {
    if (grid && grid->GetNumChildren() > 0) grid->GetChild(0)->Focus(true);
  }

  int focused_tile() {
    Rml::Element *e = context->GetFocusElement();
    return e ? e->GetAttribute<int>("index", -1) : -1;
  }

  int tile_index_at(float x, float y) {
    for (Rml::Element *e = context->GetElementAtPoint({x, y}); e; e = e->GetParentNode())
      if (e->IsClassSet("tile")) return e->GetAttribute<int>("index", -1);
    return -1;
  }

  SDL_Renderer *sdl_renderer = nullptr;
  std::unique_ptr<RenderInterface> render_iface;
  std::unique_ptr<SystemInterface_SDL> system;
  std::unique_ptr<rmlbench::AssetFiles> files;
  Rml::Context *context = nullptr;
  Rml::ElementDocument *document = nullptr;
  Rml::Element *grid = nullptr;
  Rml::DataModelHandle model;
  bench::ExplorerState state;
  bench::LongPress press;
  std::vector<ItemVM> items;
  double now = 0;
};

}  // namespace

bench::Stack *bench::create_stack() { return new RmlStack(); }