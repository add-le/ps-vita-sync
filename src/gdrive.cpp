extern "C" {
#include <psp2/json.h>
#include <psp2/sysmodule.h>

#include <stdlib.h>
#include <string.h>

#include "browser.h"
#include "gdrive.hpp"
#include "httpnet.h"
#include "logger.h"
}

#include "json.hpp"

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

OAuth2Response_t handleTheAuthorizationServerResponse(char *json) {
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

  // Handle rate limit exceeded
  char *error_code = jsonGet(val, "error_code");
  if (strcmp(error_code, "rate_limit_exceeded") == 0) {
    logger_printf("Rate limit exceeded, please wait a moment and retry");
    free(error_code);

    res = initializer.terminate();
    if (res != 0) {
      logger_printf("todo 2");
      logger_panic(res);
      logger_exit(1);
    }
    sceSysmoduleUnloadModule(SCE_SYSMODULE_JSON);
  }

  char *device_code = jsonGet(val, "device_code");
  char *user_code = jsonGet(val, "user_code");
  char *expires_in = jsonGet(val, "expires_in"); // int
  char *interval = jsonGet(val, "interval");     // int
  char *verification_url = jsonGet(val, "verification_url");

  OAuth2Response_t response = {.device_code = device_code,
                               .user_code = user_code,
                               .verification_url = verification_url,
                               .expires_in = expires_in,
                               .interval = interval};

  res = initializer.terminate();
  if (res != 0) {
    logger_printf("todo 2");
    logger_panic(res);
    logger_exit(1);
  }

  sceSysmoduleUnloadModule(SCE_SYSMODULE_JSON);

  return response;
}

void freeOAuth2Response(OAuth2Response_t response) {
  free(response.device_code);
  free(response.user_code);
  free(response.verification_url);
  free(response.expires_in);
  free(response.interval);
}

void displayUserCode(OAuth2Response_t oauth2Response) {
  logger_printf("logger %s %s %s %s %s", oauth2Response.device_code,
                oauth2Response.user_code, oauth2Response.verification_url,
                oauth2Response.expires_in, oauth2Response.interval);

  char buffer[2048];

  sprintf(buffer,
          "https://add-le.github.io/ps-vita-sync/"
          "?user_code=%s&expires_in=%s&verification_url=%s",
          oauth2Response.user_code, oauth2Response.expires_in,
          oauth2Response.verification_url);

  openUrl(buffer);
}

void pollAuthorizationServer() {

  char buffer[2048];

  sprintf(buffer,
          "https://oauth2.googleapis.com/token?client_id=%s&client_secret"
          "=%s&device_code=%s&grant_type=urn%3Aietf%3Aparams%3Aoauth%3A"
          "grant-type%3Adevice_code");

  httpPost(buffer);
}