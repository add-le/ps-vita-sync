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
    this->setChildren(children);
  }
}

Path::Path(char *filename) { this->filename = filename; };

Path::~Path() {
  for (int i = 0; i < this->children.size(); i++) {
    delete this->children.at(i);
  }
  delete[] this->filepath;
}

char *Path::getFilename() { return this->filename; }
char *Path::getFilepath() { return this->filepath; }
std::vector<Path *> Path::getChildren() { return this->children; }
Path *Path::getParent() { return this->parent; }
bool Path::isFolder() { return this->_isFolder; }
bool Path::isSelected() { return this->_isSelected; }

void Path::setFilename(char *filename) { this->filename = filename; }
void Path::setFilepath(char *filepath) {
  delete[] this->filepath;
  this->filepath = new char[strlen(filepath) + 1];
  strcpy(this->filepath, filepath);
}
void Path::setChildren(std::vector<Path *> children) {
  this->children = children;
  for (int i = 0; i < this->children.size(); i++) {
    this->children.at(i)->setParent(this);
    if (!this->filepath)
      this->setFilepath(this->filename);
    char buf[1024];
    strcpy(buf, this->filepath);
    strcat(buf, "/");
    strcat(buf, this->children.at(i)->getFilename());
    strcat(buf, "/");
    this->children.at(i)->setFilepath(buf);
  }
}
void Path::setParent(Path *parent) { this->parent = parent; }
void Path::setFolder() { this->_isFolder = true; }
void Path::select() { this->_isSelected = true; }
void Path::deselect() { this->_isSelected = false; }
void Path::toggleSelected() { this->_isSelected = !this->_isSelected; }

void Path::selectAll() {
  for (int i = 0; i < this->children.size(); i++) {
    this->children.at(i)->select();
    this->children.at(i)->deselectAll();
  }
}

void Path::deselectAll() {
  for (int i = 0; i < this->children.size(); i++) {
    this->children.at(i)->deselect();
    this->children.at(i)->deselectAll();
  }
}

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
    if (file == NULL) {
      logger_printf("Failed to malloc file\n");
      logger_exit(1);
    }
    memset(file, 0, sizeof(File_t));
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