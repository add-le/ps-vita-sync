// Shared data for the reference screen: same items, icons and strings in every stack.
#pragma once

#include <string>
#include <vector>

namespace bench {

struct Item {
  std::string name;
  const char *icon;  // UTF-8 Material Symbols code point
  bool folder;
  bool synced;
};

// Material Symbols code points (see src/symbols.h)
#define ICON_FOLDER "\ue2c7"
#define ICON_NOTE "\ue06f"
#define ICON_IMAGE "\ue3f4"
#define ICON_HEADPHONES "\uf01f"
#define ICON_DATA_OBJECT "\uead3"
#define ICON_CSS "\ueb93"
#define ICON_PHP "\ueb8f"
#define ICON_HTML "\ueb7e"
#define ICON_SAVE "\ue161"
#define ICON_ARROW_BACK "\ue5c4"
#define ICON_CLOSE "\ue5cd"
#define ICON_CLOUD_SYNC "\ueb5a"
#define ICON_SELECT_ALL "\ue162"
#define ICON_CHECK_CIRCLE "\ue86c"
#define ICON_DOWNLOAD_DONE "\uf091"

// The one string that must render in a CJK font.
#define JAPANESE_LABEL "セーブデータ（ペルソナ4 ザ・ゴールデン）"

constexpr int ITEM_COUNT = 120;

inline std::vector<Item> make_items() {
  static const struct { const char *name; const char *icon; bool folder; } seed[] = {
    {"app", ICON_FOLDER, true},
    {"data", ICON_FOLDER, true},
    {"Images", ICON_FOLDER, true},
    {"Video", ICON_FOLDER, true},
    {"config.php", ICON_PHP, false},
    {"style.css", ICON_CSS, false},
    {"index.html", ICON_HTML, false},
    {"translation.json", ICON_DATA_OBJECT, false},
    {"horse.mp3", ICON_HEADPHONES, false},
    {"france.jpg", ICON_IMAGE, false},
    {"This is a name file very long very very long name file in fact it is too long name file.txt", ICON_NOTE, false},
    {"WWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWW", ICON_NOTE, false},
  };
  std::vector<Item> items;
  items.push_back({JAPANESE_LABEL, ICON_SAVE, true, true});
  for (int i = 0; (int)items.size() < ITEM_COUNT; i++) {
    const auto &s = seed[i % (sizeof(seed) / sizeof(seed[0]))];
    std::string name = s.name;
    if (i >= (int)(sizeof(seed) / sizeof(seed[0])))
      name = "PCSE" + std::to_string(10000 + i) + " " + name;
    items.push_back({name, s.icon, s.folder, i % 5 == 0});
  }
  return items;
}

}  // namespace bench