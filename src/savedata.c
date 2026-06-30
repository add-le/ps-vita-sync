#include <psp2/ctrl.h>
#include <psp2/io/dirent.h>
#include <psp2/io/fcntl.h>
#include <psp2/kernel/processmgr.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "appgame.h"
#include "logger.h"

void freeGames(char **games, unsigned short games_length) {
  for (int i = 0; i < games_length; i++) {
    free(games[i]);
  }
}

void setColor() { printf("\033[30m\033[47m"); }

void resetColor() { printf("\033[0m"); }

void printDisplay(unsigned short item, char **games,
                  unsigned short games_length, int *selected_games) {
  // printf("\e[?25l");
  // printf("\033[H\033[J");

  printf("\033[%d;%dH", 10, 0);

  for (int i = 0; i < games_length; i++) {
    if (item == i) {
      setColor();
    }

    if (selected_games[i] == 1) {
      printf("%c ", 254);
    } else {
      printf("  ");
    }

    printf("%s\n", games[i]);
    resetColor();
  }
}

void displaySavedataFolder(char *path) {
  int res;

  int fd = sceIoDopen(path);
  if (fd < 0) {
    logger_printf("Failed to open the PSP2 savedata folder (0x%x)\n", fd);
    logger_panic(fd);
    logger_exit(1);
  }

  printf("\e[0;33m");
  printf("\n x: add folder or file into ps vita sync cloud\n");
  printf(" o: remove folder or file from ps vita sync cloud\n");
  printf(" /\\: confirm selected games to be sync with ps vita sync cloud\n");
  printf("^v: move between files or folders\n");
  printf("\e[0;37m");

  printf("\n[PS Vita Games]\n");

  char *games[256];
  int selected_games[256];
  unsigned short i = 0;
  unsigned short item = 0;

  SceIoDirent dir;
  while (sceIoDread(fd, &dir) > 0) {
    // Display game save folder
    const char *game_title = findNameById("descript.ion", dir.d_name);
    if (game_title != NULL) {
      char *buffer = (char *)malloc(sizeof(char) * 2048);
      sprintf(buffer, "%s%c %s", dir.d_name, 124, game_title);
      games[i] = buffer;
      i++;
      free((void *)game_title);
    } else {
      printf("ID %s not found.\n", dir.d_name);
    }
  }

  // Init selected_games
  for (int j = 0; j < i; j++) {
    selected_games[j] = 0;
  }

  printDisplay(0, games, i, selected_games);
  SceCtrlData ctrl;
  while (1) {
    sceCtrlPeekBufferPositive(0, &ctrl, 1);
    if (ctrl.buttons == SCE_CTRL_DOWN && item < i - 1) {
      item++;
      printDisplay(item, games, i, selected_games);
      sceKernelDelayThread(0.2 * 1000000);
    } else if (ctrl.buttons == SCE_CTRL_UP && item > 0) {
      item--;
      printDisplay(item, games, i, selected_games);
      sceKernelDelayThread(0.2 * 1000000);
    } else if (ctrl.buttons == SCE_CTRL_CROSS) {
      selected_games[item] = 1;
      printDisplay(item, games, i, selected_games);
      sceKernelDelayThread(0.2 * 1000000);
    } else if (ctrl.buttons == SCE_CTRL_CIRCLE) {
      selected_games[item] = 0;
      printDisplay(item, games, i, selected_games);
      sceKernelDelayThread(0.2 * 1000000);
    } else if (ctrl.buttons == SCE_CTRL_TRIANGLE) {
      /** TODO: Returns selected games */
      break;
    }
  }

  freeGames(games, i);
  res = sceIoDclose(fd);
  if (res < 0) {
    logger_printf("Failed to close PSP2 savedata folder (0x%x)\n", fd);
    logger_panic(res);
    logger_exit(1);
  }
}
