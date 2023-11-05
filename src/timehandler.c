#define _XOPEN_SOURCE

#include <psp2/io/dirent.h>

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "timehandler.h"

#include "debugScreen.h"

#define printf psvDebugScreenPrintf

long long psp2DateTimeToMs(SceDateTime date_time) {
  struct tm timeinfo = {0};
  timeinfo.tm_year = date_time.year - 1900; // Adjust year
  timeinfo.tm_mon = date_time.month - 1;    // Adjust month
  timeinfo.tm_mday = date_time.day;
  timeinfo.tm_hour = date_time.hour;
  timeinfo.tm_min = date_time.minute;
  timeinfo.tm_sec = date_time.second;

  printf("year %hu month %hu day %hu hour %hu minute %hu second %hu",
         date_time.year, date_time.month, date_time.day, date_time.hour,
         date_time.minute, date_time.second);

  time_t rawtime = mktime(&timeinfo);
  if (rawtime == -1) {
    printf("error on mktime");
    return -1;
  }

  long long milliseconds = (long long)rawtime * 1000;
  return milliseconds;
}

unsigned long long int ISOStringToMs(char *iso_string) {
  struct tm tm = {0};
  strptime(iso_string, "%Y-%m-%dT%H:%M:%S", &tm);
  time_t epoch = mktime(&tm);
  return (unsigned long long int)epoch * 1000;
}
