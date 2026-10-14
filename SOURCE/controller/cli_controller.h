/*
   The commands: SYNC downloads every page of a language, SHOW prints a page. The exit
   code is 0 when it worked, 1 when it did not.
*/

#ifndef TTGWIKI_CONTROLLER_CLI_CONTROLLER_H
#define TTGWIKI_CONTROLLER_CLI_CONTROLLER_H

class CliView;
class Error;
class PageIndex;
class ReaderService;
class SyncService;
struct Config;

class CliController {
public:
  CliController( SyncService &sync, ReaderService &reader, PageIndex &index, CliView &view, Config &config,
                 Error &error );
  /* lang: NULL for the one of the Config */
  int sync( const char *lang );
  int show( const char *route, const char *lang );

private:
  void useLanguage( const char *lang );
  bool loadIndex();
  int fail();

  SyncService &sync_;
  ReaderService &reader_;
  PageIndex &index_;
  CliView &view_;
  Config &config_;
  Error &error_;
};

#endif
