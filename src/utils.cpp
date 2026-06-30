#include "utils.hpp"

#include <algorithm>

bool hasEnding(std::string &fullString, std::string &ending) {

  std::transform(fullString.begin(), fullString.end(), fullString.begin(),
                 ::tolower);
  std::transform(ending.begin(), ending.end(), ending.begin(), ::tolower);

  if (fullString.length() >= ending.length()) {
    return (0 == fullString.compare(fullString.length() - ending.length(),
                                    ending.length(), ending));
  } else {
    return false;
  }
}