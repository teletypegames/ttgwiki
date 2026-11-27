/*
   A text file of a language on the local disk, read a line at a time: it can be bigger
   than the memory. Failures go into the Error.
*/

#ifndef TTGWIKI_REPOSITORY_LINE_FILE_H
#define TTGWIKI_REPOSITORY_LINE_FILE_H

class LineFile {
public:
  virtual bool open( const char *lang, const char *name ) = 0;
  /* The next line without its line end, terminated: false at the end of the file. A
     line longer than size - 1 is cut. */
  virtual bool next( char *buf, unsigned size ) = 0;
  virtual void close() = 0;
};

#endif
