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
  sceSysmoduleLoadModule(SCE_SYSMODULE_HTTPS);

  SceNetInitParam net_init_param;
  int size = 16 * 1024;
  net_init_param.memory = malloc(size);
  net_init_param.size = size;
  net_init_param.flags = 0;

  int res = sceNetInit(&net_init_param);
  if (res < 0) {
    printf("sceNetInit failed (0x%X)\n", res);
    exit(1);
  }

  res = sceNetCtlInit();
  if (res < 0) {
    printf("sceNetCtlInit failed (0x%X)\n", res);
    exit(1);
  }

  res = sceSslInit(512 * 1024);
  if (res < 0) {
    printf("sceSslInit failed (0x%X)\n", res);
    exit(1);
  }

  res = sceHttpInit(64 * 1024);
  if (res < 0) {
    printf("sceHttpInit failed (0x%X)\n", res);
    exit(1);
  }
}

HttpResponse_t _httpSend(char *url, SceHttpMethods method,
                         HttpHeader_t *headers, size_t headers_length) {
  int res;

  int tpl = sceHttpCreateTemplate("PS Vita Sync", SCE_HTTP_VERSION_1_1, 1);
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

  int statusCode;
  res = sceHttpGetStatusCode(req, &statusCode);
  if (res < 0) {
    printf("sceHttpGetStatusCode failed (0x%X)\n", res);
    exit(1);
  }

  char *recv_buffer = NULL;
  unsigned long long int contentLength = 0L;

  if (statusCode == 200) {
    res = sceHttpGetResponseContentLength(req, &contentLength);
    printf("content length (0x%x) %d\n\n", res, contentLength);
    if (res >= 0 || contentLength > 0) {
      // Content length set
      recv_buffer = (char *)malloc(contentLength);
      sceHttpReadData(req, recv_buffer, contentLength);
    } else if (res == SCE_HTTP_ERROR_NO_CONTENT_LENGTH ||
               res == SCE_HTTP_ERROR_CHUNK_ENC) {
      // Assuming no content length means chunk mode
      unsigned char data[16 * 1024];
      int read;
      int prev_read = 0;

      while ((read = sceHttpReadData(req, &data, sizeof(data))) > 0) {
        recv_buffer = (char *)realloc(recv_buffer, read);
        memcpy(recv_buffer + prev_read, data, read);
        prev_read = read;
      }
    } else {
      printf("sceHttpGetStatusCode failed (0x%X)\n", res);
      exit(1);
    }
  } else {
    printf("request failed HTTP %d\n", statusCode);
    exit(1);
  }

  HttpResponse_t response = {.buffer = recv_buffer, .length = contentLength};
  return response;
}

HttpResponse_t httpGet(char *url, HttpHeader_t *headers,
                       size_t headers_length) {
  return _httpSend(url, SCE_HTTP_METHOD_GET, headers, headers_length);
}

HttpResponse_t httpPost(char *url) {
  return _httpSend(url, SCE_HTTP_METHOD_POST, NULL, 0);
}

void freeHttpResponse(HttpResponse_t response) { free(response.buffer); }
