/*
   The page being read, in the language of the Config: brought to the disk when it is
   not there yet, and read from there a line at a time.
*/

#ifndef TTGWIKI_SERVICE_READER_SERVICE_H
#define TTGWIKI_SERVICE_READER_SERVICE_H

class PageSource;
class SyncService;
struct Config;

class ReaderService {
public:
  ReaderService( SyncService &sync, PageSource &source, const Config &config );
  /* false: the page cannot be had (the Error says why). */
  bool open( unsigned id );
  void close();
  bool isOpen() const;
  unsigned id() const;
  int lineCount() const;
  bool truncated() const;
  bool readLine( int line, char *buf, unsigned size );
  bool hasLinks( int line ) const;
  const char *title() const;
  const char *url() const;
  /* The target of link n: "#<id>" for a page of the wiki, or a URL. */
  bool linkTarget( int link, char *out, unsigned size );

private:
  SyncService &sync_;
  PageSource &source_;
  const Config &config_;
};

#endif
