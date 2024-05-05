extern "C" {
#include "logger.h"
#include <vita2d.h>
}

#include "draw.hpp"

void draw_rounded_rectangle(uint x, uint y, uint w, uint h, uint r0,
                            uint color) {
  draw_rounded_rectangle(x, y, w, h, r0, r0, r0, r0, color);
}

void draw_rounded_rectangle(uint x, uint y, uint w, uint h, uint r0, uint r1,
                            uint color) {
  draw_rounded_rectangle(x, y, w, h, r0, r1, r0, r1, color);
}

void draw_rounded_rectangle(uint x, uint y, uint w, uint h, uint r0, uint r1,
                            uint r2, uint color) {
  draw_rounded_rectangle(x, y, w, h, r0, r1, r2, r1, color);
}

void draw_rounded_rectangle(uint x, uint y, uint w, uint h, uint r0, uint r1,
                            uint r2, uint r3, uint color) {

  vita2d_draw_fill_circle(x + r0, y + r0, r0, color);
  vita2d_draw_fill_circle(x + w - r1, y + r1, r1, color);
  vita2d_draw_fill_circle(x + w - r2, y + h - r2, r2, color);
  vita2d_draw_fill_circle(x + r3, y + h - r3, r3, color);

  vita2d_draw_rectangle(x + r0, y, w - r1 - r0, h / 2, color);
  vita2d_draw_rectangle(x + r3, y + h / 2, w - r2 - r3, h / 2, color);
  vita2d_draw_rectangle(x, y + r0, w / 2, h - r3 - r0, color);
  // If w is odd, need extra pixel (always ?)
  vita2d_draw_rectangle(x + w / 2, y + r1,
                        (w / 2) % 2 == 0 ? (w / 2) : (w / 2) + 1, h - r2 - r1,
                        color);
}
