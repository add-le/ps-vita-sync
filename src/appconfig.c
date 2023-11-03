#include "appconfig.h"
#include <psp2/io/fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

AppConfig_t *loadConfig() {
  char config_location[31];
  sprintf(config_location, "ux0:/data/%s/config.lst", APP_ID);

  int fd = sceIoOpen(config_location, SCE_O_RDONLY, 0777);
  if (fd < 0) {
    return NULL;
  }

  int fd_size = sceIoLseek(fd, 0, SCE_SEEK_END);
  sceIoLseek(fd, 0, SCE_SEEK_SET);

  char *config_file = (char *)malloc(fd_size + 1);
  int br = sceIoRead(fd, config_file, fd_size);
  if (br <= 0) {
    return NULL;
  }
  config_file[fd_size] = '\0';

  AppConfig_t *config = (AppConfig_t *)malloc(sizeof(AppConfig_t));

  char iss[256];
  char kid[256];

  FILE *fp = fmemopen(config_file, strlen(config_file), "r");
  if (fp == NULL) {
    return NULL;
  }

  fscanf(fp, "iss\t%s\nkid\t%s\n", iss, kid);

  config->iss = (char *)malloc(strlen(iss) + 1);
  strcpy(config->iss, iss);
  config->kid = (char *)malloc(strlen(kid) + 1);
  strcpy(config->kid, kid);

  sceIoClose(fd);
  fclose(fp);
  return config;
}

void freeConfig(AppConfig_t *config) {
  free(config->iss);
  free(config->kid);
  free(config);
}
