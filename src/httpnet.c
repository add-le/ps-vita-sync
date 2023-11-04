#include <psp2/kernel/sysmem.h>
#include <psp2/libssl.h>
#include <psp2/net/http.h>
#include <psp2/net/netctl.h>
#include <psp2/paf.h>
#include <psp2/sysmodule.h>

#include <stdlib.h>
#include <string.h>

#include "debugScreen.h"
#include "httpnet.h"

#define printf psvDebugScreenPrintf

void httpNetInit() {
  int res;
  SceUInt32 paf_init_param[6];
  SceSysmoduleOpt sysmodule_opt;

  paf_init_param[0] = 0x4000000;
  paf_init_param[1] = 0;
  paf_init_param[2] = 0;
  paf_init_param[3] = 0;
  paf_init_param[4] = 0x400;
  paf_init_param[5] = 1;

  res = ~0;
  sysmodule_opt.flags = 0;
  sysmodule_opt.result = &res;

  sceSysmoduleLoadModuleInternalWithArg(SCE_SYSMODULE_INTERNAL_PAF,
                                        sizeof(paf_init_param), &paf_init_param,
                                        &sysmodule_opt);
  sceSysmoduleLoadModule(SCE_SYSMODULE_HTTPS);

  SceNetInitParam net_init_param;
  net_init_param.size = 0x800000;
  net_init_param.flags = 0;

  SceUID memid = sceKernelAllocMemBlock("SceNetMemory", 0x0C20D060,
                                        net_init_param.size, NULL);
  if (memid < 0) {
    printf("sceKernelAllocMemBlock failed (0x%X)\n", memid);
    exit(1);
  }

  sceKernelGetMemBlockBase(memid, &net_init_param.memory);

  res = sceNetInit(&net_init_param);
  if (res < 0) {
    printf("sceNetInit failed (0x%X)\n", res);
    exit(1);
  }

  res = sceNetCtlInit();
  if (res < 0) {
    printf("sceNetCtlInit failed (0x%X)\n", res);
    exit(1);
  }

  res = sceHttpInit(0x800000);
  if (res < 0) {
    printf("sceHttpInit failed (0x%X)\n", res);
    exit(1);
  }

  res = sceSslInit(0x800000);
  if (res < 0) {
    printf("sceSslInit failed (0x%X)\n", res);
    exit(1);
  }
}

char *_httpSend(char *url, SceHttpMethods method, HttpHeader_t *headers,
                size_t headers_length) {
  int res;

  int tpl = sceHttpCreateTemplate("PS Vita Sync", 2, 1);
  if (tpl < 0) {
    printf("sceHttpCreateTemplate failed (0x%X)\n", tpl);
    exit(1);
  }

  // Add headers to the request
  if (headers != NULL && headers_length > 0) {
    for (int i = 0; i < headers_length; i++) {
      res = sceHttpAddRequestHeader(tpl, headers[i].key, headers[i].value,
                                    SCE_HTTP_HEADER_ADD);
    }
  }

  int conn = sceHttpCreateConnectionWithURL(tpl, url, 0);
  if (conn < 0) {
    printf("sceHttpCreateConnectionWithURL failed (0x%X)\n", conn);
    exit(1);
  }

  int req = sceHttpCreateRequestWithURL(conn, method, url, 0);
  if (req < 0) {
    printf("sceHttpCreateRequestWithURL failed (0x%X)\n", req);
    exit(1);
  }

  res = sceHttpSendRequest(req, NULL, 0);
  if (res < 0) {
    printf("sceHttpSendRequest failed (0x%X)\n", res);
    exit(1);
  }

  char *recv_buffer = sce_paf_memalign(0x40, 0x40000);
  sceHttpReadData(req, recv_buffer, 0x40000);

  return recv_buffer;
}

char *httpGet(char *url, HttpHeader_t *headers, size_t headers_length) {
  return _httpSend(url, SCE_HTTP_METHOD_GET, headers, headers_length);
}

char *httpPost(char *url) {
  return _httpSend(url, SCE_HTTP_METHOD_POST, NULL, 0);
}
