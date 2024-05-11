#ifndef _FS_HPP_
#define _FS_HPP_

#include <vector>

class Path {
public:
  Path(char *filename);
  Path(char *filename, std::vector<Path *> children);
  ~Path();

  char *getFilename();
  std::vector<Path *> getChildren();
  Path *getParent();
  bool isFolder();

  void setFilename(char *filename);
  void setChildren(std::vector<Path *> children);
  void setParent(Path *parent);
  void setFolder();

private:
  char *filename;
  std::vector<Path *> children;
  Path *parent;
  bool _isFolder = false;
};

struct FSFile {
  char filename[256];
  int isFolder;
};

typedef struct FSFile File_t;

std::vector<File_t *> ls(char *path);

std::vector<Path *> folder(char *path);

#endif
