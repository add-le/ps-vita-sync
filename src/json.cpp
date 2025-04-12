
#include <psp2/json.h>

#include "json.hpp"

#include <malloc.h>
#include <string.h>

char *jsonGet(sce::Json::Value &json_root, const char *key) {
  sce::Json::String string = sce::Json::String();
  const sce::Json::Value &value = json_root.getValue(key);
  value.toString(string);
  const char *c_string = string.c_str();
  char *caser = (char *)malloc(strlen(c_string));
  strcpy(caser, c_string);
  return caser;
}