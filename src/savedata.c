#include "logger.h"
#include <inttypes.h>
#include <psp2/io/dirent.h>
#include <psp2/io/fcntl.h>
#include <psp2/kernel/processmgr.h>
#include <stdio.h>
#include <string.h>

void displaySavedataFolder(char *path) {
  int fd = sceIoDopen(path);
  if (fd < 0) {
    logger_printf("Failed to open the PSP2 savedata folder (0x%x)\n", fd);
    logger_panic(fd);
    logger_exit(1);
  }

  SceIoDirent dir;
  while (sceIoDread(fd, &dir) > 0) {
    logger_printf("file %s %" PRId64 "\n", dir.d_name, dir.d_stat.st_size);

    char file_path[1024];
    sprintf(file_path, "%s%s", path, dir.d_name);

    int file = sceIoDopen(file_path);
    if (file < 0) {
      logger_printf("TODO (0x%x)\n", file);
      logger_panic(file);
      logger_exit(1);
    }

    SceIoDirent file_dir;
    short savedata_files = 0;
    while (sceIoDread(file, &file_dir) > 0) {
      savedata_files++;
      logger_printf("TODO 2 %s %" PRId64 "\n", file_dir.d_name,
                    file_dir.d_stat.st_size);
    }

    if (savedata_files != 0) {
      logger_printf("TODO 3 %d\n", savedata_files);
    }
  }

  int res = sceIoDclose(fd);
  if (res < 0) {
    logger_printf("Failed to close PSP2 savedata folder (0x%x)\n", fd);
    logger_panic(res);
    logger_exit(1);
  }
}
