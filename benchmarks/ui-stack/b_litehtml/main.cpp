// Stack B: litehtml (layout only) + ContainerSDL (drawing). litehtml has no scripting, data
// binding, events beyond hit-testing, scrolling inertia or animation, so this file does what the
// engine does not: builds HTML from state, scrolls, hit-tests, moves focus, animates the header.
#include <algorithm>
#include <cmath>
#include <memory>
#include <string>
#include <vector>

#include "container_sdl.h"
#include "explorer_state.h"
#include "harness.h"

namespace {

constexpr float GRID_TOP = 60;  // header area, reserved like the prototype's in-flow header
constexpr int COLUMNS = 4;

std::string escape(const std::string &s) {
  std::string out;
  for (char c : s) {
    if (c == '<') out += "&lt;";
    else if (c == '>') out += "&gt;";
    else if (c == '&') out += "&amp;";
    else out += c;
  }
  return out;
}

struct Box {
  float x, y, w, h;
  bool contains(float px, float py, float margin = 0) const {
    return px >= x - margin && px < x + w + margin && py >= y - margin && py < y + h + margin;
  }
};

Box box(const litehtml::element::ptr &el) {
  litehtml::position p = el->get_placement();
  return {p.x, p.y, p.width, p.height};
}

float back_out(float t) {  // easing close to the prototype's cubic-bezier(0.175, 0.885, 0.32, 1.275)
  const float s = 1.70158f;
  t -= 1;
  return t * t * ((s + 1) * t + s) + 1;
}

class LitehtmlStack : public bench::Stack {
public:
  const char *name() const override { return "B-litehtml"; }

  bool init(SDL_Renderer *r, int w, int h, const char *assets) override {
    renderer = r;
    width = w;
    height = h;
    container = std::make_unique<ContainerSDL>(r, assets, w, h);
    css = container->read_asset("explorer.css");
    if (css.empty()) return false;
    state.open_root();
    build_grid();
    build_header();
    return true;
  }

  void button(bench::Button b, bool down) override {
    if (!down) return;
    int n = (int)state.entries.size();
    int f = focus;
    switch (b) {
      case bench::Button::Up: f -= COLUMNS; break;
      case bench::Button::Down: f += COLUMNS; break;
      case bench::Button::Left: f -= 1; break;
      case bench::Button::Right: f += 1; break;
      case bench::Button::Cross: apply(state.tap(focus)); return;
      case bench::Button::Circle: apply(state.back()); return;
      case bench::Button::Triangle: apply(state.toggle(focus)); return;
      default: return;
    }
    if (f < 0 || f >= n) return;
    set_focus(f);
  }

  void front_touch(const bench::Touch &t) override {
    if (t.kind == bench::Touch::Down) {
      dragging = false;
      velocity = 0;
      touch_y = touch_y0 = t.y;
      touch_t = now;
      press.start(t.x, t.y, now, tile_at(t.x, t.y));
    } else if (t.kind == bench::Touch::Move) {
      press.move(t.x, t.y);
      if (!dragging && std::fabs(t.y - touch_y0) > 12) dragging = true;
      if (dragging) {
        float dy = t.y - touch_y;
        scroll_by(-dy);
        double dt = std::max(1e-3, now - touch_t);
        velocity = 0.7f * velocity + 0.3f * (float)(-dy / dt);
      }
      touch_y = t.y;
      touch_t = now;
    } else {
      press.end();
      if (dragging || press.consumed()) return;
      if (header_t > 0.5f && t.y < GRID_TOP) tap_header(t.x, t.y);
      else apply(state.tap(tile_at(t.x, t.y)));
    }
  }

  void scroll_by(float dy) override {
    float max_scroll = std::max(0.0f, (float)grid->height() - (height - GRID_TOP));
    scroll = std::clamp(scroll + dy, 0.0f, max_scroll);
  }

  void toggle_select(int index) override { apply(state.toggle(index)); }

  void update(double t) override {
    double dt = now == 0 ? 0 : t - now;
    now = t;
    int target = press.poll(t);
    if (target >= 0) apply(state.toggle(target));
    // Touch fling.
    if (!dragging && std::fabs(velocity) > 5) {
      scroll_by((float)(velocity * dt));
      velocity *= (float)std::exp(-4.0 * dt);
    }
    // Header transition: 0.5 s, like the prototype.
    float goal = state.selected_count > 0 ? 1.0f : 0.0f;
    float step = (float)dt / 0.5f;
    header_t = goal > header_t ? std::min(goal, header_t + step) : std::max(goal, header_t - step);
  }

  void render() override {
    SDL_Rect clip{0, (int)GRID_TOP, width, height - (int)GRID_TOP};
    litehtml::position pos(0, GRID_TOP, (float)width, (float)height - GRID_TOP);
    container->alpha = 1;
    container->base_clip = clip;
    SDL_RenderSetClipRect(renderer, &clip);
    grid->draw(0, 0, GRID_TOP - scroll, &pos);
    container->base_clip = SDL_Rect{0, 0, 0, 0};
    SDL_RenderSetClipRect(renderer, nullptr);
    if (header_t > 0) {
      float y = -50 + 50 * back_out(header_t);
      container->alpha = header_t;
      litehtml::position hpos(0, 0, (float)width, GRID_TOP);
      header->draw(0, 0, y, &hpos);
    }
  }

private:
  void build_grid() {
    std::string html = "<html><head><style>" + css + "</style></head><body><div id=\"grid\">";
    for (size_t i = 0; i < state.entries.size(); i++) {
      const auto &e = state.entries[i];
      html += "<div class=\"tile";
      if (e.selected) html += " selected";
      if ((int)i == focus) html += " focused";
      html += "\"><span class=\"icon\"><span class=\"glyph\">";
      html += e.icon;
      html += "</span><span class=\"check\">" ICON_CHECK_CIRCLE "</span>";
      if (e.synced) html += "<span class=\"sync\">" ICON_DOWNLOAD_DONE "</span>";
      html += "</span><span class=\"label\">" + escape(e.name) + "</span></div>";
    }
    html += "</div></body></html>";
    grid = litehtml::document::createFromString(html, container.get());
    grid->render((float)width);
    auto found = grid->root()->select_all(".tile");
    tiles.assign(found.begin(), found.end());
  }

  void build_header() {
    std::string count = std::to_string(state.selected_count) + (state.selected_count > 1 ? " items" : " item");
    std::string html = "<html><head><style>" + css + "</style></head><body><div id=\"header\">"
                       "<div class=\"group\"><span id=\"close\" class=\"button\">" ICON_CLOSE "</span>"
                       "<span>" + count + "</span></div>"
                       "<div class=\"group\"><span class=\"button\">" ICON_CLOUD_SYNC "</span>"
                       "<span id=\"selall\" class=\"button\">" ICON_SELECT_ALL "</span></div></div></body></html>";
    header = litehtml::document::createFromString(html, container.get());
    header->render((float)width);
  }

  // litehtml only re-evaluates selectors that matched when the document was parsed
  // (html_tag::refresh_styles walks m_used_styles), so adding a class such as "selected" at
  // runtime does not restyle. Any state change therefore rebuilds the whole document.
  void apply(bench::ExplorerState::Result r) {
    if (r == bench::ExplorerState::None) return;
    if (r == bench::ExplorerState::Navigated) {
      focus = 0;
      scroll = 0;
    }
    build_grid();
    build_header();
  }

  void set_focus(int f) {
    focus = f;
    build_grid();
    // Scroll the focused tile into view.
    Box p = box(tiles[focus]);
    float view = height - GRID_TOP;
    if (p.y < scroll) scroll = p.y - 10;
    else if (p.y + p.h > scroll + view) scroll = p.y + p.h - view + 10;
    scroll_by(0);
  }

  int tile_at(float x, float y) {
    if (y < GRID_TOP) return -1;
    float dy = y - GRID_TOP + scroll;
    for (size_t i = 0; i < tiles.size(); i++)
      if (box(tiles[i]).contains(x, dy)) return (int)i;
    return -1;
  }

  void tap_header(float x, float y) {
    auto hit = [&](const char *sel) {
      auto el = header->root()->select_one(sel);
      return el && box(el).contains(x, y, 8);
    };
    if (hit("#close")) apply(state.select_all(false));
    else if (hit("#selall")) apply(state.select_all(true));
  }

  SDL_Renderer *renderer = nullptr;
  int width = 0, height = 0;
  std::unique_ptr<ContainerSDL> container;
  std::string css;
  litehtml::document::ptr grid, header;
  std::vector<litehtml::element::ptr> tiles;
  bench::ExplorerState state;
  bench::LongPress press;
  int focus = 0;
  float scroll = 0, velocity = 0, touch_y = 0, touch_y0 = 0, header_t = 0;
  double now = 0, touch_t = 0;
  bool dragging = false;
};

}  // namespace

bench::Stack *bench::create_stack() { return new LitehtmlStack(); }