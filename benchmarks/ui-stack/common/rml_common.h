// RmlUi helpers shared by stacks A and C: asset-relative file access and font loading.
#pragma once

#include <RmlUi/Core.h>

#include <cstdio>
#include <string>
#include <vector>

namespace rmlbench {

inline std::vector<Rml::byte> read_file(const std::string &path) {
  std::vector<Rml::byte> data;
  FILE *f = fopen(path.c_str(), "rb");
  if (!f) return data;
  fseek(f, 0, SEEK_END);
  data.resize((size_t)ftell(f));
  fseek(f, 0, SEEK_SET);
  if (fread(data.data(), 1, data.size(), f) != data.size()) data.clear();
  fclose(f);
  return data;
}

// Resolves every RmlUi path against the asset directory, so documents use plain
// relative names ("explorer.rcss") on both host and Vita ("app0:/").
class AssetFiles : public Rml::FileInterface {
public:
  explicit AssetFiles(std::string base) : base(std::move(base)) {}
  Rml::FileHandle Open(const Rml::String &path) override {
    std::string full = path.rfind(base, 0) == 0 ? path : base + path;
    return (Rml::FileHandle)fopen(full.c_str(), "rb");
  }
  void Close(Rml::FileHandle f) override { fclose((FILE *)f); }
  size_t Read(void *buf, size_t size, Rml::FileHandle f) override { return fread(buf, 1, size, (FILE *)f); }
  bool Seek(Rml::FileHandle f, long offset, int origin) override { return fseek((FILE *)f, offset, origin) == 0; }
  size_t Tell(Rml::FileHandle f) override { return (size_t)ftell((FILE *)f); }

private:
  std::string base;
};

// Fonts are loaded from memory so documents can use short family names.
// Noto Sans JP is registered as the fallback face: any glyph missing from Kanit (CJK) comes from it.
inline bool load_fonts(const std::string &assets) {
  static std::vector<Rml::byte> kanit, icons, japanese;
  kanit = read_file(assets + "Kanit-Regular.ttf");
  icons = read_file(assets + "MaterialSymbols-Regular.ttf");
  japanese = read_file(assets + "NotoSansJP-Regular.ttf");
  using S = Rml::Style::FontStyle;
  using W = Rml::Style::FontWeight;
  return !kanit.empty() && !icons.empty() && !japanese.empty() &&
         Rml::LoadFontFace(kanit, "kanit", S::Normal, W::Normal, false) &&
         Rml::LoadFontFace(icons, "icons", S::Normal, W::Normal, false) &&
         Rml::LoadFontFace(japanese, "notojp", S::Normal, W::Normal, true);
}

}  // namespace rmlbench