extern "C" {
#include <psp2/io/fcntl.h>
#include <psp2/json.h>
#include <psp2/sysmodule.h>

#include <stdlib.h>
#include <string.h>

#include "appconfig.h"
#include "logger.h"
}

#include "gdrive.hpp"
#include "json.hpp"
#include "oauth.hpp"

OAuthCredentials_t loadOAuthCredentials() {

  char config_location[256];
  sprintf(config_location, "ux0:/data/%s/config.json", APP_ID);

  int fd = sceIoOpen(config_location, SCE_O_RDONLY, 0777);
  if (fd < 0) {
    logger_printf("Failed to open file 'config.json' (0x%x)\n", fd);
    logger_panic(fd);
    logger_exit(1);
  }

  int fd_size = sceIoLseek(fd, 0, SCE_SEEK_END);
  sceIoLseek(fd, 0, SCE_SEEK_SET);

  char *config_file = (char *)malloc(fd_size + 1);
  int br = sceIoRead(fd, config_file, fd_size);
  if (br <= 0) {
  }
  config_file[fd_size] = '\0';

  sceSysmoduleLoadModule(SCE_SYSMODULE_JSON);

  int res;
  Allocator *memAllocator = new Allocator();
  const sce::Json::InitParameter initParams = {.allocator = memAllocator,
                                               .bufSize = 256};

  sce::Json::Initializer initializer = sce::Json::Initializer();
  res = initializer.initialize(&initParams);
  if (res != 0) {
    logger_printf("todo");
    logger_panic(res);
    logger_exit(1);
  }

  sce::Json::Value val = sce::Json::Value();
  res = sce::Json::Parser::parse(val, config_file, strlen(config_file));
  if (res != 0) {
    logger_printf("todo 3");
    logger_panic(res);
    logger_exit(1);
  }

  logger_printf("logger client id %s", jsonGet(val, "client_id"));
  logger_printf("logger client secret %s", jsonGet(val, "client_secret"));

  sceSysmoduleUnloadModule(SCE_SYSMODULE_JSON);

  free(config_file);

  res = initializer.terminate();
  if (res != 0) {
    logger_printf("todo 2");
    logger_panic(res);
    logger_exit(1);
  }

  sceSysmoduleUnloadModule(SCE_SYSMODULE_JSON);
}

void freeOAuthCredentials(OAuthCredentials_t credentials) {
  free(credentials.client_id);
  free(credentials.client_secret);
}
