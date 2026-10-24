// Baseline: the harness with a stack that draws nothing. Size and memory deltas of the
// other stacks are measured against this.
#include "harness.h"

namespace {

class EmptyStack : public bench::Stack {
public:
  const char *name() const override { return "0-empty"; }
  bool init(SDL_Renderer *, int, int, const char *) override { return true; }
  void button(bench::Button, bool) override {}
  void front_touch(const bench::Touch &) override {}
  void scroll_by(float) override {}
  void toggle_select(int) override {}
  void update(double) override {}
  void render() override {}
};

}  // namespace

bench::Stack *bench::create_stack() { return new EmptyStack(); }