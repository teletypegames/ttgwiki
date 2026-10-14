/*
   A page on the local disk, read a line at a time: the file can be bigger than the
   memory. A page file starts with header lines (TITLE=, ROUTE=, LOCALE=, MODIFIED=,
   URL=, and L=<n> <target> for each link), then an empty line, then the lines of the
   page (see model/page_line.h). Failures go into the Error.
*/

#ifndef TTGWIKI_REPOSITORY_PAGE_SOURCE_H
#define TTGWIKI_REPOSITORY_PAGE_SOURCE_H

class PageSource {
public:
  virtual bool open( const char *lang, unsigned id ) = 0;
  virtual void close() = 0;
  virtual unsigned id() const = 0;
  /* The lines of the page (without the header). */
  virtual int lineCount() const = 0;
  /* The page had more lines than fit; the rest is left out. */
  virtual bool truncated() const = 0;
  /* A line in the code page of the screen, terminated (the control bytes kept). */
  virtual bool readLine( int line, char *buf, unsigned size ) = 0;
  /* The line has a link (cheap: known from opening the page). */
  virtual bool hasLinks( int line ) const = 0;
  virtual const char *title() const = 0;
  virtual const char *url() const = 0;
  /* The target of link n: "#<id>" for a page of the wiki, or a URL. */
  virtual bool linkTarget( int link, char *out, unsigned size ) = 0;
};

#endif
