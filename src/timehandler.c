#define _XOPEN_SOURCE

#include <psp2/io/dirent.h>

#include <stdio.h>
#include <time.h>

#include "timehandler.h"

unsigned long long int psp2DateTimeToMs(SceDateTime date_time) {
  unsigned long long int milliseconds = 0;
  milliseconds += date_time.year * 365 * 24 * 60 * 60 * 1000;
  milliseconds += date_time.month * 30 * 24 * 60 * 60 * 1000;
  milliseconds += date_time.day * 24 * 60 * 60 * 1000;
  milliseconds += date_time.hour * 60 * 60 * 1000;
  milliseconds += date_time.minute * 60 * 1000;
  milliseconds += date_time.second * 1000;
  return milliseconds;
}

unsigned long long int ISOStringToMs(char *iso_string) {
  struct tm tm = {0};
  strptime(iso_string, "%Y-%m-%dT%H:%M:%S", &tm);
  time_t epoch = mktime(&tm);
  return (unsigned long long int)epoch * 1000;
}
