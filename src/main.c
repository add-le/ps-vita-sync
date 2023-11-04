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

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "debugScreen.h"
#include "driveapi.h"
#include "httpnet.h"
#include "savefile.h"

#define printf psvDebugScreenPrintf
#define wait3s sceKernelDelayThread(3 * 1000000)

int main(int argc, char *argv[]) {
  // Inits
  psvDebugScreenInit();
  OpenSSL_add_all_algorithms();
  httpNetInit();

  char *access_token = getOAuth2Token();
  char *mhf2ndgId = "18LgbwEbFMuC_VIYZ6bL1wYnGXPaSzqSj";
  // // char *json = getFile(mhf2ndgId, "?fields=modifiedTime", false,
  // // access_token);

  // // unsigned long long int lmt_driveFile = getModifiedTime(json->buffer);
  // // free(json);

  // // unsigned long long int lmt_localFile =
  // //
  // getLastModificationTime("ux0:/pspemu/PSP/SAVEDATA/ULUS10391/MHP2NDG.BIN");

  HttpResponse_t res = downloadSavefile(mhf2ndgId, access_token);

  printf("res : %llu", res.length);

  free(access_token);
  freeHttpResponse(res);

  wait3s;
  sceKernelExitProcess(0);
}
