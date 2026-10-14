/*
   The browser: the contents (arrows move, typing searches the titles, Enter reads, F2
   changes the language, F5 refreshes, F6 downloads every page, Esc quits) and the
   reader (arrows and paging scroll, Tab / Shift+Tab select the next / previous link,
   Enter follows it, Backspace goes back, F2 shows the page in the other language, F5
   refreshes, Esc returns to the contents).
*/

#ifndef TTGWIKI_CONTROLLER_BROWSER_CONTROLLER_H
#define TTGWIKI_CONTROLLER_BROWSER_CONTROLLER_H

#include "../lib/ui/list_cursor.h"
#include "../model/history.h"
#include "../model/page_index.h"
#include "../view/wiki_view.h"

class Error;
class Idle;
class Keyboard;
class ReaderService;
class SyncService;
struct Config;
struct LinkSpot;

class BrowserController {
public:
  enum { LineSize = 400, QuerySize = 32 };

  BrowserController( SyncService &sync, ReaderService &reader, PageIndex &index, WikiView &view, Keyboard &keyboard,
                     Idle &idle, Config &config, Error &error );
  void run();

private:
  enum Mode { Contents, Reading };

  unsigned waitKey();

  void rebuildVisible( unsigned keepId );
  unsigned selectedId() const;
  void drawContents();
  bool contentsKey( unsigned key );
  void moveSelection( unsigned char scan );
  void typeQuery( unsigned char ch );
  void refresh();
  void switchLanguage();
  void downloadAll();

  bool openPage( unsigned id, int top );
  void drawPage();
  bool readerKey( unsigned key );
  void scroll( unsigned char scan );
  void clampTop();
  void clearSelection();
  void selectLink( int line, const LinkSpot &spot );
  void showLine( int line );
  void nextLink();
  void previousLink();
  void followLink();
  void back();
  void toContents();
  void showSelectedTarget();
  void showPageMessage();

  SyncService &sync_;
  ReaderService &reader_;
  PageIndex &index_;
  WikiView &view_;
  Keyboard &keyboard_;
  Idle &idle_;
  Config &config_;
  Error &error_;

  Mode mode_;
  ListCursor cursor_;
  int visible_[PageIndex::MaxPages];
  int visibleCount_;
  char query_[QuerySize];

  History history_;
  int top_;
  int selLine_, selColumn_, selLength_, selLink_;
  char lines_[WikiView::TextRows][LineSize];
  const char *linePointers_[WikiView::TextRows];
};

#endif
