#ifndef TTGWIKI_REPOSITORY_FS_FS_PAGE_STORE_H
#define TTGWIKI_REPOSITORY_FS_FS_PAGE_STORE_H

#include "../page_store.h"
#include "../../lib/net/file_sink.h"

class Error;
class FileSystem;

/* <root>\<lang>\<name>; the temporary file is <root>\<lang>\_NEW.TMP. */
class FsPageStore : public PageStore {
public:
  FsPageStore( FileSystem &fs, Error &error );
  /* root: the cache folder */
  void setRoot( const char *root );
  bool prepare( const char *lang );
  int readText( const char *lang, const char *name, char *buf, unsigned size );
  bool writeText( const char *lang, const char *name, const char *text );
  bool has( const char *lang, const char *name );
  void remove( const char *lang, const char *name );
  FileSink *beginFile( const char *lang );
  bool endFile( const char *lang, const char *name, bool keep );
  void path( const char *lang, const char *name, char *out );

private:
  FileSystem &fs_;
  Error &error_;
  FileSink file_;
  char root_[80];
};

#endif
