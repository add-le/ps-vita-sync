// Shared benchmark harness: SDL2 window/renderer, input mapping, scripted run,
// frame-time and memory statistics, on-screen overlay, results file.
#pragma once

#define SDL_MAIN_HANDLED
#include <SDL.h>

namespace bench {

enum class Button { Up, Down, Left, Right, Cross, Circle, Triangle, Square, L, R, Select, Start };

// Front-panel touch in screen pixels. `id` is the finger id.
struct Touch {
  enum Kind { Down, Move, Up } kind;
  long long id;
  float x, y;
};

// One stack under test. All stacks draw through the same SDL_Renderer.
class Stack {
public:
  virtual ~Stack() = default;
  virtual const char *name() const = 0;
  // Asset directory ends with '/': "app0:/" on Vita, the source assets folder on host.
  virtual bool init(SDL_Renderer *renderer, int width, int height, const char *assets) = 0;
  virtual void button(Button b, bool down) = 0;
  virtual void front_touch(const Touch &t) = 0;
  // Scroll the grid by `dy` pixels (rear touch and the scripted run use this).
  virtual void scroll_by(float dy) = 0;
  // Toggle selection of item `index` (the scripted run uses this to drive the header transition).
  virtual void toggle_select(int index) = 0;
  virtual void update(double now_seconds) = 0;
  virtual void render() = 0;
};

Stack *create_stack();

}  // namespace bench