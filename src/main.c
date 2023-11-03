#include <psp2/io/fcntl.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/libssl.h>

#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "appconfig.h"
#include "base64.h"
#include "debugScreen.h"
#include "jwt64.h"
#include "pkey.h"
#include "rs256.h"
#include "sha256.h"

#define printf psvDebugScreenPrintf
#define wait3s sceKernelDelayThread(3 * 1000000)

#define EXPIRATION_TIME 3600

int main(int argc, char *argv[]) {
  // Inits
  psvDebugScreenInit();
  OpenSSL_add_all_algorithms();

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

  printf("jwt %s\n", jwt);

  freeConfig(config);
  wait3s;
  exit(0);
}
