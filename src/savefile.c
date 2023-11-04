#include <psp2/io/dirent.h>
#include <psp2/io/fcntl.h>
#include <psp2/paf.h>
#include <psp2kern/kernel/utils.h>

#include <stdbool.h>
#include <stdlib.h>

#include "driveapi.h"
#include "httpnet.h"
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

int downloadSavefile(char *fileId, char *access_token) {
  HttpResponse_t drive_file = getFile(fileId, NULL, true, access_token);

  // Backup the local_file
  int res = sceIoRename("ux0:/pspemu/PSP/SAVEDATA/ULUS10391/MHP2NDG.BIN",
                        "ux0:/pspemu/PSP/SAVEDATA/ULUS10391/MHP2NDG.BIN.old");
  if (res < 0) {
    return res;
  }

  int fd;
  if ((fd = sceIoOpen("ux0:/pspemu/PSP/SAVEDATA/ULUS10391/MHP2NDG.BIN",
                      SCE_O_WRONLY | SCE_O_CREAT, 0777)) < 0) {
    return fd;
  }

  int bw = sceIoWrite(fd, drive_file.buffer, drive_file.length);
  if (bw != drive_file.length) {
    return -1;
  }

  sceIoClose(fd);

  freeHttpResponse(drive_file);
  return 1;
}

int uploadSavefile(char *path, char *access_token) {}
