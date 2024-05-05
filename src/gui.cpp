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

#include "draw.hpp"
#include "fs.hpp"
#include "gui.hpp"

#define TILE_WIDTH 231
#define TILE_HEIGHT 48

#define GRID_MARGIN 10
#define GRID_GAP 6
#define GRID_ROW 4

#define FONT_SIZE 14
#define SYMBOLS_SIZE 24

#define FOLDER_PADDING_X 16
#define FOLDER_PADDING_Y 12

#define BORDER_RADIUS 12

#define OFFSET_Y 2

Path *root = nullptr;
vita2d_font *font = NULL;
vita2d_font *symbols = NULL;
bool running = true;

struct Id2DBox {
  int x;
  int y;
  int h;
  int w;
  int id;
};
typedef struct Id2DBox Id2DBox_t;

std::vector<Id2DBox_t *> clickEvents;

struct _vector2 {
  int x;
  int y;
};
typedef struct _vector2 vector2;

vector2 old_touch;

void freeClickEvents() {
  for (int i = 0; i < clickEvents.size(); i++) {
    free(clickEvents.at(i));
  }
  clickEvents.clear();
}

void guiPathTile(int x, int y, char *text, bool isFolder) {
  draw_rounded_rectangle(x, y, TILE_WIDTH, TILE_HEIGHT, BORDER_RADIUS, ACCENT);
  vita2d_font_draw_text(
      font,
      x + FOLDER_PADDING_X * 2 +
          vita2d_font_text_width(symbols, SYMBOLS_SIZE, FOLDER),
      y + TILE_HEIGHT / 2 + vita2d_font_text_height(font, FONT_SIZE, text) / 2 -
          OFFSET_Y,
      LIGHT_BLACK, FONT_SIZE, text);
  if (isFolder) {
    vita2d_font_draw_text(
        symbols, x + FOLDER_PADDING_X,
        y + FOLDER_PADDING_Y +
            vita2d_font_text_height(symbols, SYMBOLS_SIZE, FOLDER),
        GRAY, SYMBOLS_SIZE, FOLDER);
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

    int x = GRID_MARGIN + TILE_WIDTH * row + GRID_GAP * row;
    int y = GRID_MARGIN + TILE_HEIGHT * column + GRID_GAP * column;
    guiPathTile(x, y, root->getChildren().at(i)->getFilename(),
                root->getChildren().at(i)->isFolder());

    // Bind click event
    Id2DBox_t *event = (Id2DBox_t *)malloc(sizeof(Id2DBox_t));
    event->x = x;
    event->y = y;
    event->w = TILE_WIDTH;
    event->h = TILE_HEIGHT;
    event->id = i;
    clickEvents.push_back(event);

    row++;
  }
}

void openFolder(int id) {
  freeClickEvents();

  char *parent = root->getFilename();
  char *child = root->getChildren().at(id)->getFilename();

  int path_length = strlen(parent) + strlen(child) + 2;
  char *path = (char *)malloc(path_length);
  strcpy(path, parent);
  strcat(path, child);
  strcat(path, "/");

  std::vector<Path *> children = folder(path);
  root = new Path(path, children);
}

void handleClickEvent(SceTouchData *touch) {
  bool touched = false;
  for (int i = 0; i < clickEvents.size(); i++) {
    if (touch->report[0].x >= clickEvents.at(i)->x * 2 &&
        touch->report[0].x <=
            (clickEvents.at(i)->x * 2 + clickEvents.at(i)->w * 2) &&
        touch->report[0].y >= clickEvents.at(i)->y * 2 &&
        touch->report[0].y <=
            (clickEvents.at(i)->y * 2 + clickEvents.at(i)->h * 2) &&
        old_touch.x == 0 && old_touch.y == 0) {
      touched = true;
      if (root->getChildren().at(clickEvents.at(i)->id)->isFolder()) {
        openFolder(clickEvents.at(i)->id);
      }
      break;
    }
  }
  if (!touched) {
    // ####
  }
}

int rectangle() {

  sceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT,
                           SCE_TOUCH_SAMPLING_STATE_START);
  sceTouchEnableTouchForce(SCE_TOUCH_PORT_FRONT);

  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");

  vita2d_init();
  font = vita2d_load_font_file("app0:/Kanit-Regular.ttf");
  symbols = vita2d_load_font_file("app0:/MaterialSymbols-Regular.ttf");

  std::vector<Path *> children = folder("ux0:/");
  root = new Path("ux0:/", children);

  SceTouchData touch;
  old_touch = {.x = 0, .y = 0};

  vita2d_set_clear_color(WHITE);
  while (running) {

    vita2d_start_drawing();
    vita2d_clear_screen();

    old_touch.x = touch.report[0].x;
    old_touch.y = touch.report[0].y;
    sceTouchPeek(0, &touch, 1);

    if (root != nullptr) {
      guiGrid(root->getChildren().size());
    }

    // Handle touch event on the screen
    handleClickEvent(&touch);

    vita2d_end_drawing();
    vita2d_swap_buffers();
  }

  vita2d_fini();
  vita2d_free_font(font);
  vita2d_free_font(symbols);

  delete root;

  return 0;
}
