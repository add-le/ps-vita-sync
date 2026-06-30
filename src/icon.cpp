#include <string>
#include <vector>

#include "icon.hpp"
#include "symbols.h"
#include "utils.hpp"

const char *getIcon(char *filename) {
  std::vector<std::string> code = {".xml"};
  for (std::string ext : code) {
    std::string s_filename = filename;
    if (hasEnding(s_filename, ext)) {
      return CODE;
    }
  }

  std::vector<std::string> php = {".php"};
  for (std::string ext : php) {
    std::string s_filename = filename;
    if (hasEnding(s_filename, ext)) {
      return PHP;
    }
  }

  std::vector<std::string> css = {".css"};
  for (std::string ext : css) {
    std::string s_filename = filename;
    if (hasEnding(s_filename, ext)) {
      return CSS;
    }
  }

  std::vector<std::string> html = {".html", ".htm"};
  for (std::string ext : html) {
    std::string s_filename = filename;
    if (hasEnding(s_filename, ext)) {
      return HTML;
    }
  }

  std::vector<std::string> javascript = {".js", ".cjs", ".mjs"};
  for (std::string ext : javascript) {
    std::string s_filename = filename;
    if (hasEnding(s_filename, ext)) {
      return JAVASCRIPT;
    }
  }

  std::vector<std::string> image = {".png",  ".jpg",  ".jpeg", ".bmp",
                                    ".gif",  ".webp", ".svg",  ".apng",
                                    ".avif", ".ico",  ".tiff", ".tif"};
  for (std::string ext : image) {
    std::string s_filename = filename;
    if (hasEnding(s_filename, ext)) {
      return IMAGE;
    }
  }

  std::vector<std::string> headphones = {".mp3", ".ogg", ".wav", ".flac",
                                         ".m4a", ".wma", ".webm"};
  for (std::string ext : headphones) {
    std::string s_filename = filename;
    if (hasEnding(s_filename, ext)) {
      return HEADPHONES;
    }
  }

  std::vector<std::string> font_download = {".ttf", ".otf", ".woff", ".woff2"};
  for (std::string ext : font_download) {
    std::string s_filename = filename;
    if (hasEnding(s_filename, ext)) {
      return FONT_DOWNLOAD;
    }
  }

  std::vector<std::string> description = {".txt"};
  for (std::string ext : description) {
    std::string s_filename = filename;
    if (hasEnding(s_filename, ext)) {
      return DESCRIPTION;
    }
  }

  std::vector<std::string> data_object = {".json"};
  for (std::string ext : data_object) {
    std::string s_filename = filename;
    if (hasEnding(s_filename, ext)) {
      return DATA_OBJECT;
    }
  }

  return NOTE;
}