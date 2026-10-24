// litehtml document_container drawing through SDL_Renderer + SDL_ttf.
// litehtml only lays out; everything visible on screen is implemented here.
#pragma once

#include <SDL.h>
#include <SDL_ttf.h>
#include <litehtml.h>

#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

class ContainerSDL : public litehtml::document_container {
public:
  ContainerSDL(SDL_Renderer *renderer, std::string assets, int width, int height);
  ~ContainerSDL() override;

  // Alpha multiplier for everything drawn next. Used to fade the header: litehtml has no
  // opacity, transform or transition support, so animation lives in the app.
  float alpha = 1.0f;
  // Clip applied under every clip litehtml sets (the grid's viewport below the header).
  SDL_Rect base_clip{0, 0, 0, 0};
  size_t text_cache_size() const { return text_cache.size(); }
  std::string read_asset(const std::string &name) const;

  litehtml::uint_ptr create_font(const litehtml::font_description &descr, const litehtml::document *doc,
                                 litehtml::font_metrics *fm) override;
  void delete_font(litehtml::uint_ptr font) override;
  litehtml::pixel_t text_width(const char *text, litehtml::uint_ptr font) override;
  void draw_text(litehtml::uint_ptr hdc, const char *text, litehtml::uint_ptr font, litehtml::web_color color,
                 const litehtml::position &pos) override;
  litehtml::pixel_t pt_to_px(float pt) const override { return pt * 96.0f / 72.0f; }
  litehtml::pixel_t get_default_font_size() const override { return 14; }
  const char *get_default_font_name() const override { return "kanit"; }
  void draw_list_marker(litehtml::uint_ptr, const litehtml::list_marker &) override {}
  void load_image(const char *, const char *, bool) override {}
  void get_image_size(const char *, const char *, litehtml::size &sz) override { sz.width = sz.height = 0; }
  void draw_image(litehtml::uint_ptr, const litehtml::background_layer &, const std::string &,
                  const std::string &) override {}
  void draw_solid_fill(litehtml::uint_ptr hdc, const litehtml::background_layer &layer,
                       const litehtml::web_color &color) override;
  void draw_linear_gradient(litehtml::uint_ptr hdc, const litehtml::background_layer &layer,
                            const litehtml::background_layer::linear_gradient &gradient) override;
  void draw_radial_gradient(litehtml::uint_ptr, const litehtml::background_layer &,
                            const litehtml::background_layer::radial_gradient &) override {}
  void draw_conic_gradient(litehtml::uint_ptr, const litehtml::background_layer &,
                           const litehtml::background_layer::conic_gradient &) override {}
  void draw_borders(litehtml::uint_ptr hdc, const litehtml::borders &borders, const litehtml::position &draw_pos,
                    bool root) override;
  void set_caption(const char *) override {}
  void set_base_url(const char *) override {}
  void link(const std::shared_ptr<litehtml::document> &, const litehtml::element::ptr &) override {}
  void on_anchor_click(const char *, const litehtml::element::ptr &) override {}
  void on_mouse_event(const litehtml::element::ptr &, litehtml::mouse_event) override {}
  void set_cursor(const char *) override {}
  void transform_text(std::string &, litehtml::text_transform) override {}
  void import_css(std::string &text, const std::string &url, std::string &) override { text = read_asset(url); }
  void set_clip(const litehtml::position &pos, const litehtml::border_radiuses &radius) override;
  void del_clip() override;
  void get_viewport(litehtml::position &viewport) const override;
  litehtml::element::ptr create_element(const char *, const litehtml::string_map &,
                                        const std::shared_ptr<litehtml::document> &) override {
    return nullptr;
  }
  void get_media_features(litehtml::media_features &media) const override;
  void get_language(std::string &language, std::string &culture) const override {
    language = "en";
    culture = "";
  }

private:
  struct Font {
    TTF_Font *primary;   // family requested by CSS
    TTF_Font *fallback;  // Noto Sans JP, for glyphs the primary font lacks
  };
  TTF_Font *open_font(const std::string &file, int size);
  TTF_Font *pick(const Font &f, const char *text) const;
  void apply_clip();

  SDL_Renderer *renderer;
  std::string assets;
  int width, height;
  std::map<std::pair<std::string, int>, TTF_Font *> fonts;
  std::map<std::tuple<TTF_Font *, std::string, uint32_t>, std::tuple<SDL_Texture *, int, int>> text_cache;
  std::vector<litehtml::position> clips;
};