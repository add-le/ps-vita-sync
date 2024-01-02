#include <psp2/ctrl.h>
#include <psp2/kernel/processmgr.h>

#include <stdio.h>
#include <stdlib.h>

#include "browser.h"
#include "display.h"
#include "logger.h"

void setColorMainMenu() { printf("\033[30m\033[47m"); }

void resetColorMainMenu() { printf("\033[0m"); }

void printDisplayMainMenu(unsigned short selected_item, char **item_list,
                          unsigned short item_list_length) {
  printf("\033[%d;%dH", 3, 0);
  for (int i = 0; i < item_list_length; i++) {
    if (selected_item == i) {
      setColorMainMenu();
    }
    printf("%s\n", item_list[i]);
    resetColorMainMenu();
  }
}

#define ITEM_LIST_LENGTH 4

void displayMainMenu() {
  unsigned short item = 0;
  char *item_list[ITEM_LIST_LENGTH] = {
      "1. Start the synchronization", "2. Manage sync games",
      "3. Log in Google Drive Account", "4. Quit"};

  char *url = "https://add-le.github.io/ps-vita-sync/"
              "?user_code=XXXX-XXXX&expires_in=1800&verification_url="
              "https://www.google.com/device";

  printDisplayMainMenu(0, item_list, ITEM_LIST_LENGTH);
  SceCtrlData ctrl;
  while (1) {
    sceCtrlPeekBufferPositive(0, &ctrl, 1);
    if (ctrl.buttons == SCE_CTRL_DOWN && item < ITEM_LIST_LENGTH - 1) {
      item++;
      printDisplayMainMenu(item, item_list, ITEM_LIST_LENGTH);
      sceKernelDelayThread(0.2 * 1000000);
    } else if (ctrl.buttons == SCE_CTRL_UP && item > 0) {
      item--;
      printDisplayMainMenu(item, item_list, ITEM_LIST_LENGTH);
      sceKernelDelayThread(0.2 * 1000000);
    } else if (ctrl.buttons == SCE_CTRL_CROSS) {
      /** TODO: Select current menu. */
      switch (item) {
      case 3:
        // 4. Quit
        logger_exit(0);
        break;
      case 2:
        // 3. Open Google Drive Log In Module
        openUrl(url);
        break;
      default:
        break;
      }
      sceKernelDelayThread(0.2 * 1000000);
    }
  }
}
