extern "C" {
#include <psp2/io/dirent.h>

#include <stdlib.h>
#include <string.h>

#include "logger.h"
}

#include <vector>

#include "fs.hpp"

Path::Path(char *filename, std::vector<Path *> children) {
  this->filename = filename;
  if (!children.empty()) {
    this->children = children;
    for (int i = 0; i < this->children.size(); i++) {
      this->children.at(i)->setParent(this);
    }
  }
}

Path::Path(char *filename) { this->filename = filename; };

char *Path::getFilename() { return this->filename; }
std::vector<Path *> Path::getChildren() { return this->children; }
Path *Path::getParent() { return this->parent; }
bool Path::isFolder() { return this->_isFolder; }

void Path::setParent(Path *parent) { this->parent = parent; }
void Path::setFolder() { this->_isFolder = true; }

std::vector<File_t *> ls(char *path) {

  int fd = sceIoDopen(path);
  if (fd < 0) {
    logger_printf("Failed to open the PSP2 folder (0x%x)\n", fd);
    logger_panic(fd);
    logger_exit(1);
  }

  std::vector<File_t *> files;
  SceIoDirent dir;
  while (sceIoDread(fd, &dir) > 0) {
    File_t *file = (File_t *)malloc(sizeof(File_t));
    strcpy(file->filename, dir.d_name);
    file->isFolder = dir.d_stat.st_attr == SCE_SO_IFDIR;
    files.push_back(file);
  }

  int res = sceIoDclose(fd);
  if (res < 0) {
    logger_printf("Failed to close PSP2 folder (0x%x)\n", fd);
    logger_panic(res);
    logger_exit(1);
  }

  return files;
}

std::vector<Path *> folder(char *path) {
  std::vector<File_t *> gFolder = ls(path);
  std::vector<Path *> folder;
  for (int i = 0; i < gFolder.size(); i++) {
    Path *file = new Path(gFolder.at(i)->filename);
    if (gFolder.at(i)->isFolder) {
      file->setFolder();
    }
    folder.push_back(file);
  }

  return folder;
}