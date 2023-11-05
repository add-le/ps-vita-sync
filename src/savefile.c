#include <psp2/io/dirent.h>
#include <psp2/io/fcntl.h>
#include <psp2/paf.h>
#include <psp2kern/kernel/utils.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "driveapi.h"
#include "httpnet.h"
#include "savefile.h"
#include "timehandler.h"

#include <curl/curl.h>

long long getLastModificationTime(char *path) {
  int fd = sceIoDopen(path);
  if (fd < 0) {
    return fd;
  }

  int res;
  SceIoDirent *dir = (SceIoDirent *)malloc(sizeof(SceIoDirent));
  SceDateTime lastModificationTime;

  while ((res = sceIoDread(fd, dir)) > 0) {
    if (res < 0) {
      return res;
    }
    if (strcmp(dir->d_name, "MHP2NDG.BIN") == 0) {
      lastModificationTime = dir->d_stat.st_mtime;
    }
  }

  if (sceIoDclose(fd) < 0) {
    return -1;
  }

  return psp2DateTimeToMs(lastModificationTime);
}

int downloadSavefile(char *fileId, char *access_token) {
  HttpResponse_t drive_file = getFile(fileId, NULL, true, access_token);

  // Backup the local_file
  int res = sceIoRename("ux0:/pspemu/PSP/SAVEDATA/ULUS10391/MHP2NDG.BIN",
                        "ux0:/pspemu/PSP/SAVEDATA/ULUS10391/MHP2NDG.BIN.old");
  if (res < 0) {
    return res;
  }

  int fd;
  if ((fd = sceIoOpen("ux0:/pspemu/PSP/SAVEDATA/ULUS10391/MHP2NDG.BIN",
                      SCE_O_WRONLY | SCE_O_CREAT, 0777)) < 0) {
    return fd;
  }

  int bw = sceIoWrite(fd, drive_file.buffer, drive_file.length);
  if (bw != drive_file.length) {
    return -1;
  }

  sceIoClose(fd);

  freeHttpResponse(drive_file);
  return 1;
}

struct string {
  char *ptr;
  size_t len;
};

void init_string(struct string *s) {
  s->len = 0;
  s->ptr = malloc(s->len + 1);
  if (s->ptr == NULL) {
    fprintf(stderr, "malloc() failed\n");
    exit(EXIT_FAILURE);
  }
  s->ptr[0] = '\0';
}

size_t writefunc(void *ptr, size_t size, size_t nmemb, struct string *s) {
  size_t new_len = s->len + size * nmemb;
  s->ptr = realloc(s->ptr, new_len + 1);
  if (s->ptr == NULL) {
    fprintf(stderr, "realloc() failed\n");
    exit(EXIT_FAILURE);
  }
  memcpy(s->ptr + s->len, ptr, size * nmemb);
  s->ptr[new_len] = '\0';
  s->len = new_len;

  return size * nmemb;
}

int uploadSavefile(char *path, char *fileId, char *access_token) {
  const char *base_url = "https://www.googleapis.com/upload/drive/v3/files/";
  const char *base_upload = "?uploadType=media";

  char *url = (char *)malloc(strlen(base_url) + strlen(fileId) +
                             strlen(base_upload) + 1);
  strcpy(url, base_url);
  strcat(url, fileId);
  strcat(url, base_upload);

  // Read local savefile
  int fd = sceIoOpen(path, SCE_O_RDONLY, 0777);
  if (fd < 0) {
    return -1;
  }

  int fd_size = sceIoLseek(fd, 0, SCE_SEEK_END);
  sceIoLseek(fd, 0, SCE_SEEK_SET);

  char *buffer = (char *)malloc(fd_size + 1);
  int br = sceIoRead(fd, buffer, fd_size);
  if (br <= 0) {
    return -1;
  }
  buffer[fd_size] = '\0';
  sceIoClose(fd);

  char *bearer = (char *)malloc(22 + strlen(access_token) + 1);
  sprintf(bearer, "Authorization: Bearer %s", access_token);

  int length = snprintf(NULL, 0, "%d", fd_size);
  char *content_length = malloc(length + 1);
  snprintf(content_length, length + 1, "%d", fd_size);

  const char *base_cl = "Content-Length: ";
  char *cl = (char *)malloc(strlen(base_cl) + strlen(content_length) + 1);
  strcpy(cl, base_cl);
  strcat(cl, content_length);

  CURL *curl;
  CURLcode res;

  curl = curl_easy_init();
  if (curl) {
    struct curl_slist *headers = NULL;
    headers =
        curl_slist_append(headers, "Content-Type: application/octet-stream");
    headers = curl_slist_append(headers, bearer);
    headers = curl_slist_append(headers, cl);

    // Set the URL for the PATCH request
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "PS Vita Sync");
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);

    // Set the custom headers
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    // Set the custom request type to PATCH
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PATCH");

    // Set the request body
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, buffer);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, fd_size);

    // struct string s;
    // init_string(&s);

    // curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
    // curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);

    // Perform the request
    res = curl_easy_perform(curl);

    int httpresponsecode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpresponsecode);

    // Clean up and check for errors
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
      return res;
    }
  }
  curl_global_cleanup();

  free(cl);
  free(url);
  free(buffer);
  free(bearer);
  free(content_length);

  return 1;
}
