#include "pkey.h"
#include "appconfig.h"
#include <psp2/io/fcntl.h>
#include <stdio.h>
#include <stdlib.h>

char *loadPkey() {
  char pkey_location[29];
  sprintf(pkey_location, "ux0:/data/%s/pkey.pem", APP_ID);

  int fd = sceIoOpen(pkey_location, SCE_O_RDONLY, 0777);
  if (fd < 0) {
    return NULL;
  }

  int fd_size = sceIoLseek(fd, 0, SCE_SEEK_END);
  sceIoLseek(fd, 0, SCE_SEEK_SET);

  char *pkey = (char *)malloc(fd_size + 1);
  int br = sceIoRead(fd, pkey, fd_size);
  if (br <= 0) {
    return NULL;
  }
  pkey[fd_size] = '\0';

  sceIoClose(fd);

  return pkey;
}
