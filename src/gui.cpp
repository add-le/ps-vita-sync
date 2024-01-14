extern "C" {

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <psp2/kernel/processmgr.h>
#include <psp2/pgf.h>
#include <psp2/pvf.h>
#include <psp2/touch.h>

#include <vita2d.h>

#include "colors.h"
#include "logger.h"
#include "symbols.h"
}

#include <vector>

#include "fs.hpp"
#include "gui.hpp"

#define TILE_WIDTH 231
#define TILE_HEIGHT 48

#define GRID_MARGIN 10
#define GRID_GAP 6
#define GRID_ROW 4

#define FONT_SIZE 14
#define SYMBOLS_SIZE 18

#define FOLDER_PADDING_X 10
#define FOLDER_PADDING_Y 25

Path *root = nullptr;
vita2d_font *font = NULL;
vita2d_font *symbols = NULL;

void guiPathTile(int x, int y, char *text, bool isFolder) {
  vita2d_draw_rectangle(x, y, TILE_WIDTH, TILE_HEIGHT, ACCENT);
  vita2d_font_draw_text(
      font,
      x + TILE_WIDTH / 2 - vita2d_font_text_width(font, FONT_SIZE, text) / 2,
      y + TILE_HEIGHT / 2 + vita2d_font_text_height(font, FONT_SIZE, text) / 2,
      LIGHT_BLACK, FONT_SIZE, text);
  if (isFolder) {
    vita2d_font_draw_text(symbols, x + FOLDER_PADDING_X, y + FOLDER_PADDING_Y,
                          LIGHT_BLACK, SYMBOLS_SIZE, FOLDER);
  }
}

void guiGrid(int nb) {
  int row = 0;
  int column = 0;
  for (int i = 0; i < nb; i++) {
    if (i % GRID_ROW == 0 && i != 0) {
      column++;
      row = 0;
    }

    guiPathTile(GRID_MARGIN + TILE_WIDTH * row + GRID_GAP * row,
                GRID_MARGIN + TILE_HEIGHT * column + GRID_GAP * column,
                root->getChildren().at(i)->getFilename(),
                root->getChildren().at(i)->isFolder());
    row++;
  }
}

int rectangle() {

  sceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT,
                           SCE_TOUCH_SAMPLING_STATE_START);
  sceTouchEnableTouchForce(SCE_TOUCH_PORT_FRONT);

  vita2d_init();
  font = vita2d_load_font_file("app0:/Kanit-Regular.ttf");
  symbols = vita2d_load_font_file("app0:/MaterialSymbols-Regular.ttf");

  std::vector<Path *> children = folder("ux0:/");
  root = new Path("ux0:/", children);

  SceTouchData touch;

  vita2d_set_clear_color(WHITE);
  while (1) {

    vita2d_start_drawing();
    vita2d_clear_screen();

    sceTouchPeek(0, &touch, 1);

    guiGrid(children.size());

    /** TODO: Click on the GUI - WIP */
    if (touch.report[0].x > 0) {
      vita2d_set_clear_color(LIGHT_BLACK);
    } else {
      vita2d_set_clear_color(WHITE);
    }

    vita2d_end_drawing();
    vita2d_swap_buffers();
  }

  vita2d_fini();
  vita2d_free_font(font);

  for (int i = 0; i < children.size(); i++) {
    delete children.at(i);
  }
  delete root;

  return 0;
}
