#include <psp2/io/dirent.h>
#include <psp2/paf.h>
#include <psp2kern/kernel/utils.h>

#include <stdbool.h>
#include <stdlib.h>

#include "driveapi.h"
#include "savefile.h"
#include "timehandler.h"

unsigned long long int getLastModificationTime(char *path) {
  int fd = sceIoDopen(path);
  if (fd < 0) {
    return -1;
  }

  SceIoDirent *dir = (SceIoDirent *)malloc(sizeof(SceIoDirent));
  int res = sceIoDread(fd, dir);
  if (res != 0) {
    return -1;
  }

  SceDateTime lastModificationTime = dir->d_stat.st_mtime;

  return psp2DateTimeToMs(lastModificationTime);
}

char *downloadSavefile(char *fileId, char *access_token) {
  char *gzipped = getFile(fileId, NULL, true, access_token);

  // char *buffer = (char *)malloc(0x40000);
  // int res = ksceGzipDecompress(buffer, 0x40000, gzipped, NULL);
  // if (res < 0) {
  //   return res;
  // }

  return gzipped;
}
