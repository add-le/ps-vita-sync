#include <psp2/json.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "appconfig.h"
#include "driveapi.h"
#include "httpnet.h"
#include "jwt64.h"
#include "pkey.h"
#include "timehandler.h"

#define EXPIRATION_TIME 3600 // 1 hour

char *getOAuth2Token() {
  // Load user defined files
  AppConfig_t *config = loadConfig();
  char *private_key = loadPkey();

  char *header = (char *)malloc(36 + 1 + strlen(config->kid));
  sprintf(header, "{\"alg\":\"RS256\",\"typ\":\"JWT\",\"kid\":\"%s\"}",
          config->kid);

  time_t now;
  time(&now);
  char iat[20];
  sprintf(iat, "%ld", now);
  char exp[20];
  sprintf(exp, "%ld", now + EXPIRATION_TIME);

  char *payload =
      (char *)malloc(116 + 1 + strlen(config->iss) + strlen(iat) + strlen(exp));
  sprintf(payload,
          "{\"iss\":\"%s\",\"iat\":%s,\"exp\":%s,\"scope\":\"https://"
          "www.googleapis.com/auth/drive\",\"aud\":\"https://"
          "oauth2.googleapis.com/token\"}",
          config->iss, iat, exp);

  char *jwt = newJwt(header, payload, private_key);

  const char *oauth2_token =
      "https://oauth2.googleapis.com/"
      "token?grant_type=urn%3Aietf%3Aparams%3Aoauth%3Agrant-"
      "type%3Ajwt-bearer&assertion=";
  int req_len = strlen(oauth2_token) + strlen(jwt) + 1;
  char *oauth2_url = (char *)malloc(req_len);
  strcpy(oauth2_url, oauth2_token);
  strcat(oauth2_url, jwt);

  HttpResponse_t token = httpPost(oauth2_url);

  char acc_token[4 * 1024];
  char void_buffer[4 * 1024];

  sscanf(token.buffer,
         "{\"access_token\":\"%[^\"]\",\"expires_in\":%s,\"token_type\":"
         "\"Bearer\"}",
         acc_token, void_buffer);

  char *access_token = (char *)malloc(strlen(acc_token));
  strcpy(access_token, acc_token);

  free(header);
  free(payload);
  free(oauth2_url);
  free(private_key);
  freeConfig(config);
  freeHttpResponse(token);

  return access_token;
}

HttpResponse_t getFile(char *fileId, char *fields, bool download,
                       char *access_token) {
  const char *base_url = "https://www.googleapis.com/drive/v3/files/";
  const char *base_download = "?alt=media";

  int url_length = 0;
  if (fields != NULL) {
    url_length += strlen(fields);
  }
  if (download) {
    url_length += strlen(base_download);
  }

  char *url =
      (char *)malloc(strlen(base_url) + strlen(fileId) + url_length + 1);
  strcpy(url, base_url);
  strcat(url, fileId);

  if (fields != NULL) {
    strcat(url, fields);
  }
  if (download) {
    strcat(url, base_download);
  }

  char *bearer = (char *)malloc(7 + strlen(access_token) + 1);
  sprintf(bearer, "Bearer %s", access_token);

  HttpHeader_t header = {.key = "Authorization", .value = bearer};
  HttpResponse_t gFile = httpGet(url, &header, 1);

  free(bearer);
  free(url);

  return gFile;
}

unsigned long long int getModifiedTime(char *json) {
  char iso_string[1024];
  sscanf(json, "{\n  \"modifiedTime\": \"%[^\"]\n}", iso_string);

  char *date_string = (char *)malloc(strlen(iso_string));
  strcpy(date_string, iso_string);

  unsigned long long int ms = ISOStringToMs(date_string);
  free(date_string);

  return ms;
}
