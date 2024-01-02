#include <psp2/appmgr.h>
#include <psp2/apputil.h>
#include <psp2/io/fcntl.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/libssl.h>
#include <psp2/sysmodule.h>

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

#include "appconfig.h"
#include "display.h"
#include "driveapi.h"
#include "httpnet.h"
#include "logger.h"
#include "savedata.h"
#include "savefile.h"

#define wait3s sceKernelDelayThread(3 * 1000000)

int main(int argc, char *argv[]) {
  logger_init();
  printf("\e[0;32m");
  printf("PS Vita Sync v%s\n", APP_VERSION);
  printf("\e[0;37m");

  // displaySavedataFolder("ux0:/user/00/savedata/");

  displayMainMenu();

  // // Inits
  // OpenSSL_add_all_algorithms();
  // httpNetInit();
  // printf("Inits done\n");

  // char *access_token = getOAuth2Token();
  // printf("access_token granted\n");

  // char *mhf2ndgId = "18YLl-vKzWdKYC-2FGSNN949Ddt7-i5C0";
  // char *sfo_file = "18NKESueoRL1hszPCS1-Frrwr9x8qEpKO";
  // HttpResponse_t json =
  //     getFile(mhf2ndgId, "?fields=modifiedTime", false, access_token);
  // printf("mhf2ndgId modifiedTime find\n");

  // unsigned long long int lmt_driveFile = getModifiedTime(json.buffer);
  // freeHttpResponse(json);

  // long long lmt_localFile =
  //     getLastModificationTime("ux0:/pspemu/PSP/SAVEDATA/ULUS10391/");

  // printf("cloud: %llu\n", lmt_driveFile);
  // printf("local: %lld\n", lmt_localFile);

  // // Upload the local version or download the cloud version
  // if (lmt_driveFile >= lmt_localFile) {
  //   int res = downloadSavefile(mhf2ndgId,
  //                              "ux0:/pspemu/PSP/SAVEDATA/ULUS10391/MHP2NDG.BIN",
  //                              access_token);
  //   if (res == 1) {
  //     printf("MHP2NDG savefile correctly downloaded\n");
  //     res = downloadSavefile(sfo_file,
  //                            "ux0:/pspemu/PSP/SAVEDATA/ULUS10391/PARAM.SFO",
  //                            access_token);
  //     if (res == 1) {
  //       printf("param sfo savefile correctly downloaded\n");
  //     } else {
  //       printf("error on download savefile %x\n", res);
  //     }
  //   } else {
  //     printf("error on download savefile %x\n", res);
  //   }
  // } else {
  //   httpNetClose();
  //   curl_global_init(CURL_GLOBAL_ALL);
  //   int res =
  //   uploadSavefile("ux0:/pspemu/PSP/SAVEDATA/ULUS10391/MHP2NDG.BIN",
  //                            mhf2ndgId, access_token);
  //   if (res == 1) {
  //     printf("MHP2NDG savefile correctly uploaded\n");
  //   } else {
  //     printf("error on upload savefile %d\n", res);
  //     printf("curl_easy_perform() failed: %s\n",
  //     curl_easy_strerror(res));
  //   }
  // }

  // httpNetClose();
  // free(access_token);

  printf("\nSync correctly done, app will close in 3s\n");
  wait3s;
  logger_exit(0);
}
