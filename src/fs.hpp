#ifndef _FS_H_
#define _FS_H_

#include <vector>

class Path {
public:
  Path(char *filename);
  Path(char *filename, std::vector<Path *> children);

  char *getFilename();
  std::vector<Path *> getChildren();
  Path *getParent();
  bool isFolder();

  void setParent(Path *parent);
  void setFolder();

private:
  char *filename;
  std::vector<Path *> children;
  Path *parent;
  bool _isFolder;
};

struct FSFile {
  char filename[256];
  int isFolder;
};

typedef struct FSFile File_t;

std::vector<File_t *> ls(char *path);

std::vector<Path *> folder(char *path);

#endif
