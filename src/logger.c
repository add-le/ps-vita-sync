#include <psp2/kernel/processmgr.h>

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "debugScreen.h"
#include "logger.h"
#include "psp2error.h"

int logger_fd;

int logger_printf(const char *format, ...) {
  char buf[4096];

  va_list opt;
  va_start(opt, format);
  int ret = vsnprintf(buf, sizeof(buf), format, opt);
  psvDebugScreenPuts(buf);
  va_end(opt);

  return ret;
}

int logger_panic(unsigned int errno) {
  int ret;
  switch (errno) {
  case SCE_ERROR_ERRNO_ENOENT:
    ret = logger_printf("No such file or directory\n");
    break;
  default:
    ret = logger_printf("Unknown error\n");
    break;
  }
  return ret;
}

void logger_init() {
  psvDebugScreenInit();
  /** TODO: Open log file */
  logger_fd = -1;
}

void logger_exit(int status) {
  /** TODO: Close log file */
  logger_fd = -1;
  sceKernelExitProcess(status);
}
