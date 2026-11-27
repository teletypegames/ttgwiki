#ifndef TTGWIKI_REPOSITORY_FS_FS_LINE_FILE_H
#define TTGWIKI_REPOSITORY_FS_FS_LINE_FILE_H

#include "../line_file.h"

class Error;
class FileSystem;
class PageStore;

/* A file of the PageStore, read through a small buffer. */
class FsLineFile : public LineFile {
public:
  FsLineFile( FileSystem &fs, PageStore &store, Error &error );
  bool open( const char *lang, const char *name );
  bool next( char *buf, unsigned size );
  void close();

private:
  enum { BufferSize = 512 };

  bool fill();

  FileSystem &fs_;
  PageStore &store_;
  Error &error_;
  int handle_;
  char buffer_[BufferSize];
  int length_;
  int pos_;
};

#endif
