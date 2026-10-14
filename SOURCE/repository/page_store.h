/*
   The pages on the local disk: a folder for each language with the files of the export
   (INDEX.TXT, FILES.LST, WIKI.INI and the pages that were read or downloaded). A file is
   downloaded into a temporary file and only takes its name when it is complete. Failures
   go into the Error.
*/

#ifndef TTGWIKI_REPOSITORY_PAGE_STORE_H
#define TTGWIKI_REPOSITORY_PAGE_STORE_H

class FileSink;

class PageStore {
public:
  /* The language's folder, made when it is missing. */
  virtual bool prepare( const char *lang ) = 0;
  /* The file terminated in buf: its length, or -1 when it cannot be read. */
  virtual int readText( const char *lang, const char *name, char *buf, unsigned size ) = 0;
  virtual bool writeText( const char *lang, const char *name, const char *text ) = 0;
  virtual bool has( const char *lang, const char *name ) = 0;
  virtual void remove( const char *lang, const char *name ) = 0;
  /* A new temporary file to download into; NULL when it cannot be made. */
  virtual FileSink *beginFile( const char *lang ) = 0;
  /* Closes the temporary file: with keep it becomes name (replacing it), otherwise it is
     deleted. false when it could not be kept. */
  virtual bool endFile( const char *lang, const char *name, bool keep ) = 0;
  /* The full path of a file, for reading it. */
  virtual void path( const char *lang, const char *name, char *out ) = 0;
};

#endif
