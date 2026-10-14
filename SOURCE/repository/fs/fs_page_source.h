#ifndef TTGWIKI_REPOSITORY_FS_FS_PAGE_SOURCE_H
#define TTGWIKI_REPOSITORY_FS_FS_PAGE_SOURCE_H

#include "../page_source.h"
#include "../../lib/core/code_page.h"

class Error;
class FileSystem;
class PageStore;

/* Opening a page reads it through once, for where its lines start; a line is read from
   there, through a window of the file kept in memory. */
class FsPageSource : public PageSource {
public:
  enum { WindowSize = 4096, ScanSize = 2048 };

  /* offsets: room for maxLines line starts; linkBits: maxLines / 8 bytes */
  FsPageSource( FileSystem &fs, PageStore &store, Error &error, long *offsets, unsigned char *linkBits, int maxLines );
  void setCodePage( CodePage cp );
  bool open( const char *lang, unsigned id );
  void close();
  unsigned id() const;
  int lineCount() const;
  bool truncated() const;
  bool readLine( int line, char *buf, unsigned size );
  bool hasLinks( int line ) const;
  const char *title() const;
  const char *url() const;
  bool linkTarget( int link, char *out, unsigned size );

private:
  bool scan();
  void readHeader();
  bool readRaw( int line, char *buf, unsigned size );
  bool loadWindow( long from );
  void markLink( int line );

  FileSystem &fs_;
  PageStore &store_;
  Error &error_;
  CodePage cp_;
  long *offsets_;
  unsigned char *linkBits_;
  int maxLines_;
  int handle_;
  unsigned id_;
  long size_;
  int total_;     /* lines in the file, the header included */
  int bodyStart_; /* the first line of the page */
  bool truncated_;
  char title_[80];
  char url_[128];
  char window_[WindowSize];
  long windowStart_;
  unsigned windowLength_;
};

#endif
