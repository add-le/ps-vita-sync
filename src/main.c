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

#include <curl/curl.h>

#include "debugScreen.h"
#include "driveapi.h"
#include "httpnet.h"
#include "savefile.h"

#define printf psvDebugScreenPrintf
#define wait3s sceKernelDelayThread(3 * 1000000)

int main(int argc, char *argv[]) {
  printf("PS Vita Sync v0.01\n");

  // Inits
  psvDebugScreenInit();
  OpenSSL_add_all_algorithms();
  httpNetInit();
  printf("Inits done\n");

  char *access_token = getOAuth2Token();
  printf("access_token granted\n");

  char *mhf2ndgId = "18LgbwEbFMuC_VIYZ6bL1wYnGXPaSzqSj";
  HttpResponse_t json =
      getFile(mhf2ndgId, "?fields=modifiedTime", false, access_token);
  printf("mhf2ndgId modifiedTime find\n");

  unsigned long long int lmt_driveFile = getModifiedTime(json.buffer);
  freeHttpResponse(json);

  unsigned long long int lmt_localFile =
      getLastModificationTime("ux0:/pspemu/PSP/SAVEDATA/ULUS10391/MHP2NDG.BIN");

  // Upload the local version or download the cloud version
  if (lmt_driveFile >= lmt_localFile) {
    int res = downloadSavefile(mhf2ndgId, access_token);
    if (res == 1) {
      printf("MHP2NDG savefile correctly downloaded\n");
    } else {
      printf("error on download savefile %d\n", res);
    }
  } else {
    httpNetClose();
    curl_global_init(CURL_GLOBAL_ALL);
    int res = uploadSavefile("ux0:/pspemu/PSP/SAVEDATA/ULUS10391/MHP2NDG.BIN",
                             mhf2ndgId, access_token);
    if (res == 1) {
      printf("MHP2NDG savefile correctly uploaded\n");
    } else {
      printf("error on upload savefile %d\n", res);
      printf("curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    }
  }

  free(access_token);

  printf("Sync correctly done, app will close in 3s\n");
  wait3s;
  sceKernelExitProcess(0);
}
