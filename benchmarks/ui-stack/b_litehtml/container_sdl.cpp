#include "container_sdl.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <functional>

namespace {

constexpr int ARC_SEGMENTS = 6;

using ColorAt = std::function<SDL_Color(float x, float y)>;

// Outline of a box with per-corner radii, clockwise from the top-left arc.
std::vector<SDL_FPoint> rounded_outline(float x, float y, float w, float h, float tl, float tr, float br, float bl) {
  std::vector<SDL_FPoint> pts;
  auto arc = [&](float cx, float cy, float r, float a0) {
    for (int i = 0; i <= ARC_SEGMENTS; i++) {
      float a = (a0 + 90.0f * i / ARC_SEGMENTS) * (float)M_PI / 180.0f;
      pts.push_back({cx + r * std::cos(a), cy + r * std::sin(a)});
    }
  };
  float m = std::min(w, h) / 2;
  tl = std::min(tl, m); tr = std::min(tr, m); br = std::min(br, m); bl = std::min(bl, m);
  arc(x + tl, y + tl, tl, 180);
  arc(x + w - tr, y + tr, tr, 270);
  arc(x + w - br, y + h - br, br, 0);
  arc(x + bl, y + h - bl, bl, 90);
  return pts;
}

SDL_Color to_sdl(const litehtml::web_color &c, float alpha) {
  return {c.red, c.green, c.blue, (Uint8)(c.alpha * alpha)};
}

}  // namespace

ContainerSDL::ContainerSDL(SDL_Renderer *renderer, std::string assets, int width, int height)
    : renderer(renderer), assets(std::move(assets)), width(width), height(height) {
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
}

ContainerSDL::~ContainerSDL() {
  for (auto &t : text_cache) SDL_DestroyTexture(std::get<0>(t.second));
  for (auto &f : fonts) TTF_CloseFont(f.second);
}

std::string ContainerSDL::read_asset(const std::string &name) const {
  std::string out;
  FILE *f = fopen((assets + name).c_str(), "rb");
  if (!f) return out;
  char buf[4096];
  size_t n;
  while ((n = fread(buf, 1, sizeof(buf), f)) > 0) out.append(buf, n);
  fclose(f);
  return out;
}

TTF_Font *ContainerSDL::open_font(const std::string &file, int size) {
  auto key = std::make_pair(file, size);
  auto it = fonts.find(key);
  if (it != fonts.end()) return it->second;
  TTF_Font *f = TTF_OpenFont((assets + file).c_str(), size);
  fonts[key] = f;
  return f;
}

// SDL_ttf has no glyph fallback: pick, per word, the first font that has every glyph.
TTF_Font *ContainerSDL::pick(const Font &f, const char *text) const {
  const unsigned char *s = (const unsigned char *)text;
  while (*s) {
    Uint32 cp;
    int len = *s < 0x80 ? 1 : *s < 0xE0 ? 2 : *s < 0xF0 ? 3 : 4;
    cp = len == 1 ? s[0] : len == 2 ? ((s[0] & 0x1F) << 6) | (s[1] & 0x3F)
       : len == 3 ? ((s[0] & 0x0F) << 12) | ((s[1] & 0x3F) << 6) | (s[2] & 0x3F)
       : ((s[0] & 0x07) << 18) | ((s[1] & 0x3F) << 12) | ((s[2] & 0x3F) << 6) | (s[3] & 0x3F);
    if (!TTF_GlyphIsProvided32(f.primary, cp)) return f.fallback;
    s += len;
  }
  return f.primary;
}

litehtml::uint_ptr ContainerSDL::create_font(const litehtml::font_description &descr, const litehtml::document *,
                                             litehtml::font_metrics *fm) {
  int size = (int)std::lround((float)descr.size);
  std::string family = descr.family;
  const char *file = family.find("icons") != std::string::npos ? "MaterialSymbols-Regular.ttf"
                   : family.find("notojp") != std::string::npos ? "NotoSansJP-Regular.ttf"
                   : "Kanit-Regular.ttf";
  Font *font = new Font{open_font(file, size), open_font("NotoSansJP-Regular.ttf", size)};
  if (!font->primary) {
    delete font;
    return 0;
  }
  if (fm) {
    int minx, maxx, miny, maxy, advance;
    fm->font_size = (float)size;
    fm->height = (float)TTF_FontHeight(font->primary);
    fm->ascent = (float)TTF_FontAscent(font->primary);
    fm->descent = (float)-TTF_FontDescent(font->primary);
    TTF_GlyphMetrics(font->primary, 'x', &minx, &maxx, &miny, &maxy, &advance);
    fm->x_height = (float)maxy;
    TTF_GlyphMetrics(font->primary, '0', &minx, &maxx, &miny, &maxy, &advance);
    fm->ch_width = (float)advance;
    fm->draw_spaces = false;
  }
  return (litehtml::uint_ptr)font;
}

void ContainerSDL::delete_font(litehtml::uint_ptr font) { delete (Font *)font; }

litehtml::pixel_t ContainerSDL::text_width(const char *text, litehtml::uint_ptr font) {
  int w = 0;
  const Font &f = *(Font *)font;
  TTF_SizeUTF8(pick(f, text), text, &w, nullptr);
  return (float)w;
}

void ContainerSDL::draw_text(litehtml::uint_ptr, const char *text, litehtml::uint_ptr font,
                             litehtml::web_color color, const litehtml::position &pos) {
  const Font &f = *(Font *)font;
  TTF_Font *ttf = pick(f, text);
  uint32_t rgba = (color.red << 24) | (color.green << 16) | (color.blue << 8) | color.alpha;
  auto key = std::make_tuple(ttf, std::string(text), rgba);
  auto it = text_cache.find(key);
  if (it == text_cache.end()) {
    SDL_Surface *s = TTF_RenderUTF8_Blended(ttf, text, SDL_Color{color.red, color.green, color.blue, color.alpha});
    if (!s) return;
    SDL_Texture *t = SDL_CreateTextureFromSurface(renderer, s);
    it = text_cache.emplace(key, std::make_tuple(t, s->w, s->h)).first;
    SDL_FreeSurface(s);
  }
  SDL_Texture *t = std::get<0>(it->second);
  // Keep the fallback font on the primary font's baseline.
  float y = (float)pos.y + (float)(TTF_FontAscent(f.primary) - TTF_FontAscent(ttf));
  SDL_FRect dst{(float)pos.x, y, (float)std::get<1>(it->second), (float)std::get<2>(it->second)};
  SDL_SetTextureAlphaMod(t, (Uint8)(255 * alpha));
  SDL_RenderCopyF(renderer, t, nullptr, &dst);
}

static void draw_polygon(SDL_Renderer *renderer, const std::vector<SDL_FPoint> &pts, SDL_FPoint center,
                         const ColorAt &color_at) {
  std::vector<SDL_Vertex> v;
  std::vector<int> idx;
  v.reserve(pts.size() + 1);
  v.push_back({center, color_at(center.x, center.y), {0, 0}});
  for (auto &p : pts) v.push_back({p, color_at(p.x, p.y), {0, 0}});
  for (size_t i = 1; i <= pts.size(); i++) {
    idx.push_back(0);
    idx.push_back((int)i);
    idx.push_back((int)(i % pts.size()) + 1);
  }
  SDL_RenderGeometry(renderer, nullptr, v.data(), (int)v.size(), idx.data(), (int)idx.size());
}

void ContainerSDL::draw_solid_fill(litehtml::uint_ptr, const litehtml::background_layer &layer,
                                   const litehtml::web_color &color) {
  if (color.alpha == 0) return;
  const auto &b = layer.border_box;
  const auto &r = layer.border_radius;
  auto pts = rounded_outline(b.x, b.y, b.width, b.height, r.top_left_x, r.top_right_x, r.bottom_right_x,
                             r.bottom_left_x);
  SDL_Color c = to_sdl(color, alpha);
  draw_polygon(renderer, pts, {(float)(b.x + b.width / 2), (float)(b.y + b.height / 2)},
               [c](float, float) { return c; });
}

void ContainerSDL::draw_linear_gradient(litehtml::uint_ptr, const litehtml::background_layer &layer,
                                        const litehtml::background_layer::linear_gradient &g) {
  if (g.color_points.empty()) return;
  const auto &b = layer.border_box;
  const auto &r = layer.border_radius;
  auto pts = rounded_outline(b.x, b.y, b.width, b.height, r.top_left_x, r.top_right_x, r.bottom_right_x,
                             r.bottom_left_x);
  // Per-vertex colours along the gradient line: exact for two stops, approximate for more.
  float dx = g.end.x - g.start.x, dy = g.end.y - g.start.y;
  float len2 = std::max(1e-3f, dx * dx + dy * dy);
  float ox = (float)layer.origin_box.x, oy = (float)layer.origin_box.y;
  float a = alpha;
  auto color_at = [&, a](float x, float y) {
    float t = ((x - ox - g.start.x) * dx + (y - oy - g.start.y) * dy) / len2;
    const auto &cp = g.color_points;
    if (t <= cp.front().offset) return to_sdl(cp.front().color, a);
    for (size_t i = 1; i < cp.size(); i++) {
      if (t <= cp[i].offset) {
        float k = (t - cp[i - 1].offset) / std::max(1e-3f, cp[i].offset - cp[i - 1].offset);
        auto c0 = cp[i - 1].color, c1 = cp[i].color;
        auto mix = [k](int u, int v) { return (Uint8)(u + (v - u) * k); };
        return SDL_Color{mix(c0.red, c1.red), mix(c0.green, c1.green), mix(c0.blue, c1.blue),
                         (Uint8)(mix(c0.alpha, c1.alpha) * a)};
      }
    }
    return to_sdl(cp.back().color, a);
  };
  draw_polygon(renderer, pts, {(float)(b.x + b.width / 2), (float)(b.y + b.height / 2)}, color_at);
}

void ContainerSDL::draw_borders(litehtml::uint_ptr, const litehtml::borders &bd, const litehtml::position &p,
                                bool) {
  const auto &t = bd.top;
  bool uniform = (float)t.width > 0 && t.style != litehtml::border_style_none && bd.left.width == t.width &&
                 bd.right.width == t.width && bd.bottom.width == t.width && bd.left.color == t.color &&
                 bd.right.color == t.color && bd.bottom.color == t.color;
  if (uniform) {
    // Ring between the outer and the inner rounded outline.
    const auto &r = bd.radius;
    float w = t.width, x = p.x, y = p.y, pw = p.width, ph = p.height;
    float tl = r.top_left_x, tr = r.top_right_x, br = r.bottom_right_x, bl = r.bottom_left_x;
    auto outer = rounded_outline(x, y, pw, ph, tl, tr, br, bl);
    auto inner = rounded_outline(x + w, y + w, pw - 2 * w, ph - 2 * w, std::max(0.0f, tl - w),
                                 std::max(0.0f, tr - w), std::max(0.0f, br - w), std::max(0.0f, bl - w));
    SDL_Color c = to_sdl(t.color, alpha);
    std::vector<SDL_Vertex> v;
    std::vector<int> idx;
    for (size_t i = 0; i < outer.size(); i++) {
      v.push_back(SDL_Vertex{outer[i], c, SDL_FPoint{0, 0}});
      v.push_back(SDL_Vertex{inner[i], c, SDL_FPoint{0, 0}});
    }
    int n = (int)outer.size();
    for (int i = 0; i < n; i++) {
      int a = 2 * i, b = 2 * i + 1, c2 = 2 * ((i + 1) % n), d = 2 * ((i + 1) % n) + 1;
      idx.insert(idx.end(), {a, b, c2, b, d, c2});
    }
    SDL_RenderGeometry(renderer, nullptr, v.data(), (int)v.size(), idx.data(), (int)idx.size());
    return;
  }
  // Mixed borders: straight edges only.
  auto edge = [&](const litehtml::border &e, SDL_FRect rc) {
    if ((float)e.width <= 0 || e.style == litehtml::border_style_none) return;
    SDL_Color c = to_sdl(e.color, alpha);
    SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
    SDL_RenderFillRectF(renderer, &rc);
  };
  edge(bd.top, {p.x, p.y, p.width, bd.top.width});
  edge(bd.bottom, {p.x, p.y + p.height - bd.bottom.width, p.width, bd.bottom.width});
  edge(bd.left, {p.x, p.y, bd.left.width, p.height});
  edge(bd.right, {p.x + p.width - bd.right.width, p.y, bd.right.width, p.height});
}

// Rectangular clipping only: border radii on overflow clips are ignored.
void ContainerSDL::set_clip(const litehtml::position &pos, const litehtml::border_radiuses &) {
  clips.push_back(pos);
  apply_clip();
}

void ContainerSDL::del_clip() {
  if (!clips.empty()) clips.pop_back();
  apply_clip();
}

void ContainerSDL::apply_clip() {
  bool base = base_clip.w > 0 && base_clip.h > 0;
  if (clips.empty()) {
    SDL_RenderSetClipRect(renderer, base ? &base_clip : nullptr);
    return;
  }
  float x0 = 0, y0 = 0, x1 = (float)width, y1 = (float)height;
  if (base) {
    x0 = (float)base_clip.x;
    y0 = (float)base_clip.y;
    x1 = (float)(base_clip.x + base_clip.w);
    y1 = (float)(base_clip.y + base_clip.h);
  }
  for (auto &c : clips) {
    x0 = std::max(x0, (float)c.x);
    y0 = std::max(y0, (float)c.y);
    x1 = std::min(x1, (float)(c.x + c.width));
    y1 = std::min(y1, (float)(c.y + c.height));
  }
  SDL_Rect r{(int)x0, (int)y0, std::max(0, (int)(x1 - x0)), std::max(0, (int)(y1 - y0))};
  SDL_RenderSetClipRect(renderer, &r);
}

void ContainerSDL::get_viewport(litehtml::position &viewport) const {
  viewport = litehtml::position(0, 0, (float)width, (float)height);
}

void ContainerSDL::get_media_features(litehtml::media_features &media) const {
  media.type = litehtml::media_type_screen;
  media.width = media.device_width = (float)width;
  media.height = media.device_height = (float)height;
  media.color = 8;
  media.resolution = 96;
}