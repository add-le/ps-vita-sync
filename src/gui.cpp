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

#include <string>
#include <vector>

#include "draw.hpp"
#include "fs.hpp"
#include "gui.hpp"
#include "icon.hpp"

#define TILE_WIDTH 231
#define TILE_HEIGHT 48

#define HEADER_WIDTH 941
#define HEADER_HEIGHT 42

#define GRID_MARGIN 10
#define GRID_GAP 6
#define GRID_ROW 4

#define FONT_SIZE 14
#define SYMBOLS_SIZE 24

#define ICON_PADDING_X 16
#define ICON_PADDING_Y 12

#define HEADER_PADDING_X 15

#define BORDER_RADIUS 12
#define HEADER_BORDER_RADIUS 18

#define OFFSET_Y 2

#define TS_HOLD 500

/** cwd */
Path *root = nullptr;
vita2d_font *font = NULL;
vita2d_font *symbols = NULL;
bool running = true;
/** Contains number of Path currenly selected. */
uint selected = 0;

struct Id2DBox {
  int x;
  int y;
  int h;
  int w;
  int id;
};
typedef struct Id2DBox Id2DBox_t;

std::vector<Id2DBox_t *> clickEvents;
/** Events with a static (immutable) position on the GUI. */
std::vector<Id2DBox_t> staticEvents;

// List of static elements
enum StaticElements {
  CLOSE_BUTTON,
  SELECT_ALL_BUTTON,
  CLOUD_SYNC_BUTTON,
};

// Static position of header elements
// Close button (deselect all)
int closeButton_x;
int closeButton_y;
// Select all button
int selectAllButton_x;
int selectAllButton_y;
// Sync to cloud button
int cloudSyncButton_x;
int cloudSyncButton_y;

struct _vector2 {
  int x;
  int y;
};
typedef struct _vector2 vector2;

vector2 old_touch;

/** Update the view with the right number of selected Path. */
void fetchSelected() {
  std::vector<Path *> children = root->getChildren();
  uint count = 0;
  for (int i = 0; i < children.size(); i++) {
    if (children.at(i)->isSelected())
      count++;
  }
  selected = count;
}

void freeClickEvents() {
  for (int i = 0; i < clickEvents.size(); i++) {
    free(clickEvents.at(i));
  }
  clickEvents.clear();
}

void guiPathTile(int x, int y, char *text, const char *icon, uint color) {
  draw_rounded_rectangle(x, y, TILE_WIDTH, TILE_HEIGHT, BORDER_RADIUS, color);
  if (text != nullptr) {
    vita2d_font_draw_text(
        font,
        x + ICON_PADDING_X * 2 +
            vita2d_font_text_width(symbols, SYMBOLS_SIZE, icon),
        y + TILE_HEIGHT / 2 +
            vita2d_font_text_height(font, FONT_SIZE, text) / 2 - OFFSET_Y,
        LIGHT_BLACK, FONT_SIZE, text);
    vita2d_font_draw_text(
        symbols, x + ICON_PADDING_X,
        y + ICON_PADDING_Y +
            vita2d_font_text_height(symbols, SYMBOLS_SIZE, icon),
        GRAY, SYMBOLS_SIZE, icon);
  } else {
    vita2d_font_draw_text(
        symbols,
        x + TILE_WIDTH / 2 -
            vita2d_font_text_width(symbols, SYMBOLS_SIZE, icon) / 2,
        y + ICON_PADDING_Y +
            vita2d_font_text_height(symbols, SYMBOLS_SIZE, icon),
        GRAY, SYMBOLS_SIZE, icon);
  }
}

void guiHeader() {
  draw_rounded_rectangle(GRID_MARGIN, GRID_MARGIN, HEADER_WIDTH, HEADER_HEIGHT,
                         HEADER_BORDER_RADIUS, ACCENT);

  // Close button (deselect all)
  vita2d_font_draw_text(symbols, closeButton_x, closeButton_y, GRAY,
                        SYMBOLS_SIZE, CLOSE);

  char selectedCount[64];
  sprintf(selectedCount, selected > 1 ? "%d items" : "%d item", selected);

  // Selected count text
  vita2d_font_draw_text(
      font, GRID_MARGIN + HEADER_PADDING_X * 2 + closeButton_x,
      GRID_MARGIN + HEADER_HEIGHT / 2 +
          vita2d_font_text_height(font, FONT_SIZE, selectedCount) / 2 -
          OFFSET_Y,
      GRAY, FONT_SIZE, selectedCount);

  // Select all button
  vita2d_font_draw_text(symbols, selectAllButton_x, selectAllButton_y, GRAY,
                        SYMBOLS_SIZE, SELECT_ALL);

  // Sync to cloud button
  vita2d_font_draw_text(symbols, cloudSyncButton_x, cloudSyncButton_y, GRAY,
                        SYMBOLS_SIZE, CLOUD_SYNC);
}

void guiGrid(int nb, bool backButton) {
  freeClickEvents();

  if (backButton) {
    nb++;
  }

  int row = 0;
  int column = 0;
  for (int i = 0; i < nb; i++) {
    if (i % GRID_ROW == 0 && i != 0) {
      column++;
      row = 0;
    }

    int x = GRID_MARGIN + TILE_WIDTH * row + GRID_GAP * row;
    int y = HEADER_HEIGHT + GRID_MARGIN * 2 + TILE_HEIGHT * column +
            GRID_GAP * column;

    if (backButton && i == 0) {
      guiPathTile(x, y, nullptr, ARROW_BACK, ACCENT);
      // Bind click event
      Id2DBox_t *event = (Id2DBox_t *)malloc(sizeof(Id2DBox_t));
      event->x = x;
      event->y = y;
      event->w = TILE_WIDTH;
      event->h = TILE_HEIGHT;
      event->id = -1;
      clickEvents.push_back(event);
    } else {
      int j = backButton ? i - 1 : i;
      bool itemSelected = root->getChildren().at(j)->isSelected();
      guiPathTile(
          x, y, root->getChildren().at(j)->getFilename(),
          root->getChildren().at(j)->isFolder()
              ? (itemSelected ? CHECK_CIRCLE : FOLDER)
              : (itemSelected
                     ? CHECK_CIRCLE
                     : getIcon(root->getChildren().at(j)->getFilename())),
          itemSelected ? SELECTED : ACCENT);
      // Bind click event
      Id2DBox_t *event = (Id2DBox_t *)malloc(sizeof(Id2DBox_t));
      event->x = x;
      event->y = y;
      event->w = TILE_WIDTH;
      event->h = TILE_HEIGHT;
      event->id = j;
      clickEvents.push_back(event);
    }

    row++;
  }
}

void openFolder(int id) {
  freeClickEvents();

  Path *child = root->getChildren().at(id);
  if (child->getChildren().empty()) {
    std::vector<Path *> children = folder(child->getFilepath());
    child->setChildren(children);
  }
  root = child;
}

void returnBack() {
  freeClickEvents();
  root = root->getParent();
}

void selectAll() {
  root->selectAll();
  fetchSelected();
}

void deselectAll() {
  root->deselectAll();
  fetchSelected();
}

/** Init the position of the static elements and bind the click events. */
void initStaticEvents() {
  // First calculate the position of the static elements
  // Second bind the click events

  // Close button
  closeButton_x = GRID_MARGIN + HEADER_PADDING_X;
  closeButton_y = GRID_MARGIN + HEADER_HEIGHT / 2 +
                  vita2d_font_text_height(symbols, SYMBOLS_SIZE, CLOSE) / 2;

  Id2DBox closeButtonPosition = {
      .x = closeButton_x,
      .y =
          closeButton_y - vita2d_font_text_height(symbols, SYMBOLS_SIZE, CLOSE),
      .h = vita2d_font_text_height(symbols, SYMBOLS_SIZE, CLOSE),
      .w = vita2d_font_text_width(symbols, SYMBOLS_SIZE, CLOSE),
      .id = CLOSE_BUTTON};
  staticEvents.push_back(closeButtonPosition);

  // Select all button
  selectAllButton_x = HEADER_WIDTH - HEADER_PADDING_X - GRID_MARGIN;
  selectAllButton_y =
      GRID_MARGIN + HEADER_HEIGHT / 2 +
      vita2d_font_text_height(symbols, SYMBOLS_SIZE, SELECT_ALL) / 2;

  Id2DBox selectAllButtonPosition = {
      .x = selectAllButton_x,
      .y = selectAllButton_y -
           vita2d_font_text_height(symbols, SYMBOLS_SIZE, SELECT_ALL),
      .h = vita2d_font_text_height(symbols, SYMBOLS_SIZE, SELECT_ALL),
      .w = vita2d_font_text_width(symbols, SYMBOLS_SIZE, SELECT_ALL),
      .id = SELECT_ALL_BUTTON};
  staticEvents.push_back(selectAllButtonPosition);

  // Sync to cloud button
  cloudSyncButton_x = HEADER_WIDTH - HEADER_PADDING_X * 2 - GRID_MARGIN -
                      vita2d_font_text_width(symbols, SYMBOLS_SIZE, SELECT_ALL);
  cloudSyncButton_y =
      GRID_MARGIN + HEADER_HEIGHT / 2 +
      vita2d_font_text_height(symbols, SYMBOLS_SIZE, CLOUD_SYNC) / 2;

  Id2DBox cloudSyncButtonPosition = {
      .x = cloudSyncButton_x,
      .y = cloudSyncButton_y -
           vita2d_font_text_height(symbols, SYMBOLS_SIZE, CLOUD_SYNC),
      .h = vita2d_font_text_height(symbols, SYMBOLS_SIZE, CLOUD_SYNC),
      .w = vita2d_font_text_width(symbols, SYMBOLS_SIZE, CLOUD_SYNC),
      .id = CLOUD_SYNC_BUTTON};
  staticEvents.push_back(cloudSyncButtonPosition);
}

SceUInt64 previousTimeStamp = 0;
SceInt16 touched_x = 0;
SceInt16 touched_y = 0;
void handleClickEvent(SceTouchData *touch) {
  bool clicked = false;
  bool holded = false;

  SceInt16 x = touch->report[0].x;
  SceInt16 y = touch->report[0].y;

  // Touch detected
  if ((x != 0 || y != 0) && previousTimeStamp == 0) {
    // Save when touch is detected
    previousTimeStamp = touch->timeStamp;
    touched_x = x;
    touched_y = y;
  }

  // Touch released
  if (x == 0 && y == 0 && previousTimeStamp != 0) {
    SceUInt64 newTimeStamp = touch->timeStamp;

    // It's a hold touch
    if ((newTimeStamp / 1000) - (previousTimeStamp / 1000) >= TS_HOLD) {
      holded = true;
      clicked = false;
    } else {
      // It's a click touch
      holded = false;
      clicked = true;
    }

    // Touch is released, reset the previousTimeStamp
    previousTimeStamp = 0;
  }

  // Check if the touch is on a static element (header)
  // Only when header is visible (ie: selected > 0)
  if (selected > 0) {
    for (int i = 0; i < staticEvents.size(); i++) {
      if (touched_x >= staticEvents.at(i).x * 2 &&
          touched_x <= (staticEvents.at(i).x * 2 + staticEvents.at(i).w * 2) &&
          touched_y >= staticEvents.at(i).y * 2 &&
          touched_y <= (staticEvents.at(i).y * 2 + staticEvents.at(i).h * 2)) {
        if (staticEvents.at(i).id == CLOSE_BUTTON) {
          if (clicked) {
            deselectAll();
          }
        } else if (staticEvents.at(i).id == SELECT_ALL_BUTTON) {
          if (clicked) {
            selectAll();
          }
        } else if (staticEvents.at(i).id == CLOUD_SYNC_BUTTON) {
          // TODO: Implement cloud sync
        }

        break;
      }
    }
  }

  // Check if the touch is on a gui tile element
  for (int i = 0; i < clickEvents.size(); i++) {
    if (touched_x >= clickEvents.at(i)->x * 2 &&
        touched_x <= (clickEvents.at(i)->x * 2 + clickEvents.at(i)->w * 2) &&
        touched_y >= clickEvents.at(i)->y * 2 &&
        touched_y <= (clickEvents.at(i)->y * 2 + clickEvents.at(i)->h * 2)) {

      // Click on folder
      if (clickEvents.at(i)->id != -1 &&
          root->getChildren().at(clickEvents.at(i)->id)->isFolder()) {
        if (clicked && selected == 0) {
          openFolder(clickEvents.at(i)->id);
        }
        // First item to be holded and so selected
        if ((holded && selected == 0) || (clicked && selected > 0)) {
          if (selected == 0) {
            root->getChildren().at(clickEvents.at(i)->id)->select();
          } else if (selected > 0) {
            root->getChildren().at(clickEvents.at(i)->id)->toggleSelected();
          }
          fetchSelected();
        }
      }
      // Click on file
      else if (clickEvents.at(i)->id != -1 &&
               !root->getChildren().at(clickEvents.at(i)->id)->isFolder()) {
        // First item to be holded and so selected
        if ((holded && selected == 0) || (clicked && selected > 0)) {
          if (selected == 0) {
            root->getChildren().at(clickEvents.at(i)->id)->select();
          } else if (selected > 0) {
            root->getChildren().at(clickEvents.at(i)->id)->toggleSelected();
          }
          fetchSelected();
        }
      }
      // Click on back button
      else if (clickEvents.at(i)->id == -1) {
        if (clicked) {
          deselectAll();
          returnBack();
        }
      }

      break;
    }
  }
}

int guiFileExplorer() {

  sceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT,
                           SCE_TOUCH_SAMPLING_STATE_START);
  sceTouchEnableTouchForce(SCE_TOUCH_PORT_FRONT);

  vita2d_init();

  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
  SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");

  font = vita2d_load_font_file("app0:/Kanit-Regular.ttf");
  symbols = vita2d_load_font_file("app0:/MaterialSymbols-Regular.ttf");

  std::vector<Path *> children = folder("ux0:/");
  root = new Path("ux0:/", children);

  SceTouchData touch;
  old_touch = {.x = 0, .y = 0};

  // Init static events (header)
  initStaticEvents();

  vita2d_set_clear_color(WHITE);
  while (running) {
    vita2d_start_drawing();
    vita2d_clear_screen();

    old_touch.x = touch.report[0].x;
    old_touch.y = touch.report[0].y;
    sceTouchRead(0, &touch, 1);

    if (root != nullptr) {
      if (selected > 0) {
        guiHeader();
      }

      guiGrid(root->getChildren().size(), root->getParent() != nullptr);
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
