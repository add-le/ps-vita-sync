#include <psp2/apputil.h>
#include <psp2/sysmodule.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "browser.h"
#include "logger.h"

void openUrl(char *url) {
  sceSysmoduleLoadModule(SCE_SYSMODULE_APPUTIL);

  SceAppUtilInitParam *init_param =
      (SceAppUtilInitParam *)calloc(1, sizeof(SceAppUtilInitParam));
  SceAppUtilBootParam *boot_param =
      (SceAppUtilBootParam *)calloc(1, sizeof(SceAppUtilBootParam));

  SceAppUtilWebBrowserParam *browser_param =
      (SceAppUtilWebBrowserParam *)calloc(1, sizeof(SceAppUtilWebBrowserParam));
  browser_param->str = url;
  browser_param->strlen = strlen(url);

  int res = sceAppUtilInit(init_param, boot_param);
  if (res < 0) {
    logger_printf("Error %d (0x%x)", res, res);
  }

  sceAppUtilLaunchWebBrowser(browser_param);
  sceSysmoduleUnloadModule(SCE_SYSMODULE_APPUTIL);

  free(init_param);
  free(boot_param);
  free(browser_param);
}