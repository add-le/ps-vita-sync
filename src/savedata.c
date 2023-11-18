#include <psp2/io/dirent.h>
#include <psp2/io/fcntl.h>
#include <psp2/kernel/processmgr.h>

#include <stdio.h>
#include <string.h>

#include "logger.h"

void displaySavedataFolder(char *path) {
  int res;

  int fd = sceIoDopen(path);
  if (fd < 0) {
    logger_printf("Failed to open the PSP2 savedata folder (0x%x)\n", fd);
    logger_panic(fd);
    logger_exit(1);
  }

  printf("\n==== Content of savedata folder ====\n");

  SceIoDirent dir;
  while (sceIoDread(fd, &dir) > 0) {
    // Display game save folder
    printf("%s/\n", dir.d_name);

    char file_path[1024];
    sprintf(file_path, "%s%s", path, dir.d_name);

    int file = sceIoDopen(file_path);
    if (file < 0) {
      logger_printf("TODO (0x%x)\n", file);
      logger_panic(file);
      logger_exit(1);
    }

    SceIoDirent file_dir;
    while (sceIoDread(file, &file_dir) > 0) {
      printf("|-%s\n", file_dir.d_name);
    }

    res = sceIoDclose(file);
    if (res < 0) {
      logger_printf("TODO 4\n");
      logger_panic(res);
      logger_exit(1);
    }
  }

  res = sceIoDclose(fd);
  if (res < 0) {
    logger_printf("Failed to close PSP2 savedata folder (0x%x)\n", fd);
    logger_panic(res);
    logger_exit(1);
  }
}
