extern "C" {
#include <psp2/json.h>
#include <psp2/sysmodule.h>

#include <stdlib.h>
#include <string.h>

#include "gdrive.hpp"
#include "httpnet.h"
#include "logger.h"
}

/**
 * Get the value for a key in JSON object.
 * @param json_root Root value of the JSON.
 * @param key Key to get the value.
 * @return The value associated to the key.
 * @warning Result must be freed when not used anymore.
 */
char *jsonGet(sce::Json::Value &json_root, const char *key) {
  sce::Json::String string = sce::Json::String();
  const sce::Json::Value &value = json_root.getValue(key);
  value.toString(string);
  const char *c_string = string.c_str();
  char *caser = (char *)malloc(strlen(c_string));
  strcpy(caser, c_string);
  return caser;
}

HttpResponse_t requestDeviceAndUserCodes() {
  const char *scope = "https://www.googleapis.com/auth/drive.file";
  const char *client_id = "1065238718966-nat8cfknimj2s9ifau50pe0cms93c0un.apps."
                          "googleusercontent.com";
  const char *url = "https://oauth2.googleapis.com/device/code";
  size_t full_url_size = strlen(scope) + strlen(client_id) + strlen(url) +
                         strlen("?client_id=") + strlen("&scope=");

  char *full_url = (char *)malloc(full_url_size + 1);
  sprintf(full_url, "%s?client_id=%s&scope=%s", url, client_id, scope);

  HttpResponse_t response = httpPost(full_url);
  free(full_url);

  return response;
}

void handleTheAuthorizationServerResponse(char *json) {
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
  res = sce::Json::Parser::parse(val, json, strlen(json));
  if (res != 0) {
    logger_printf("todo 3");
    logger_panic(res);
    logger_exit(1);
  }

  char *device_code = jsonGet(val, "device_code");
  char *user_code = jsonGet(val, "user_code");
  char *expires_in = jsonGet(val, "expires_in"); // int
  char *interval = jsonGet(val, "interval");     // int
  char *verification_url = jsonGet(val, "verification_url");

  printf("\n device_code %s\n", device_code);
  printf("\n user_code %s\n", user_code);
  printf("\n expires_in %s\n", expires_in);
  printf("\n interval %s\n", interval);
  printf("\n verification_url %s\n", verification_url);

  free(device_code);
  free(user_code);
  free(expires_in);
  free(interval);
  free(verification_url);

  res = initializer.terminate();
  if (res != 0) {
    logger_printf("todo 2");
    logger_panic(res);
    logger_exit(1);
  }

  sceSysmoduleUnloadModule(SCE_SYSMODULE_JSON);
}
