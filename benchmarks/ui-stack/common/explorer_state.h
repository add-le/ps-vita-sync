// Reference-screen behavior shared by the C++ stacks (A, B): navigation, selection,
// long-press detection. Stack C implements the same in JavaScript (c_quickjs/app.js).
#pragma once

#include <cmath>
#include <string>
#include <vector>

#include "model.h"

namespace bench {

struct Entry {
  std::string name;
  const char *icon;
  bool folder, synced, selected, back;
  const char *shown_icon() const { return selected ? ICON_CHECK_CIRCLE : icon; }
};

class ExplorerState {
public:
  enum Result { None, SelectionChanged, Navigated };

  std::vector<Entry> entries;
  int depth = 0;
  int selected_count = 0;

  void open_root() { load(0); }

  Result tap(int i) {
    if (i < 0 || i >= (int)entries.size()) return None;
    if (selected_count > 0 && !entries[i].back) return toggle(i);
    if (entries[i].back) return back();
    if (entries[i].folder) { load(depth + 1); return Navigated; }
    return None;
  }

  Result back() {
    if (depth == 0) return None;
    load(depth - 1);
    return Navigated;
  }

  Result toggle(int i) {
    if (i < 0 || i >= (int)entries.size() || entries[i].back) return None;
    entries[i].selected = !entries[i].selected;
    selected_count += entries[i].selected ? 1 : -1;
    return SelectionChanged;
  }

  Result select_all(bool state) {
    selected_count = 0;
    for (auto &e : entries) {
      if (e.back) continue;
      e.selected = state;
      selected_count += state;
    }
    return SelectionChanged;
  }

private:
  void load(int d) {
    depth = d;
    selected_count = 0;
    entries.clear();
    if (d > 0) entries.push_back({"..", ICON_ARROW_BACK, false, false, false, true});
    auto items = make_items();
    // Sub-folders show a shorter, shifted listing so navigation visibly changes the grid.
    size_t count = d == 0 ? items.size() : items.size() / 2;
    for (size_t i = 0; i < count; i++) {
      const Item &it = items[(i + d * 7) % items.size()];
      entries.push_back({it.name, it.icon, it.folder, it.synced, false, false});
    }
  }
};

// Tap vs. long-press (500 ms, like file_explorer.js) on the front panel.
class LongPress {
public:
  void start(float x, float y, double now, int target) {
    active = true; fired = false; x0 = x; y0 = y; t0 = now; this->target = target;
  }
  void move(float x, float y) {
    if (active && std::hypot(x - x0, y - y0) > 12) active = false;
  }
  void end() { active = false; }
  // Returns the target once when the press has been held long enough.
  int poll(double now) {
    if (!active || fired || now - t0 < 0.5) return -1;
    fired = true;
    return target;
  }
  bool consumed() const { return fired; }

private:
  bool active = false, fired = false;
  float x0 = 0, y0 = 0;
  double t0 = 0;
  int target = -1;
};

}  // namespace bench