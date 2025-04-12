#include "appgame.h"
#include "logger.h"
#include <stdio.h>
#include <string.h>

#define MAX_LINE_LENGTH 256

const char *findNameById(const char *filename, const char *targetId) {
  FILE *file = fopen(filename, "r");
  if (file == NULL) {
    logger_printf("Error opening file");
    logger_exit(1);
    return NULL;
  }

  char line[MAX_LINE_LENGTH];
  while (fgets(line, sizeof(line), file) != NULL) {
    char id[MAX_LINE_LENGTH];
    char name[MAX_LINE_LENGTH];

    if (sscanf(line, "%s ^ %[^\n]", id, name) == 2) {
      if (strcmp(id, targetId) == 0) {
        fclose(file);
        return strdup(name);
      }
    }
  }

  fclose(file);
  return NULL;
}