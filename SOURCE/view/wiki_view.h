/*
   The screens of TTGWIKI: the contents (the page tree, or the pages a search found) and
   the reader of a page. Both have the title on the top line, the status line and the
   keys at the bottom. As the Progress of the operations it shows what they do in the
   status line.
*/

#ifndef TTGWIKI_VIEW_WIKI_VIEW_H
#define TTGWIKI_VIEW_WIKI_VIEW_H

#include "../lib/core/progress.h"

class Canvas;
class Error;
class ListCursor;
class PageIndex;

/* What the contents show. */
struct TreeState {
  const PageIndex *index;
  const int *visible; /* indexes into the index */
  int count;
  const ListCursor *cursor;
  const char *query; /* the search text, "" for none */
  const char *lang;
  unsigned long generation;
};

/* What the reader shows: the lines on the screen (from top), in the code page of the
   screen with their control bytes (model/page_line.h). */
struct ReaderState {
  const char *title;
  int lineCount;
  const char *const *lines;
  int shown; /* how many of lines there are */
  int top;
  int selectedLine; /* -1: no link selected */
  int selectedColumn;
  int selectedLength;
  const char *lang;
};

class WikiView : public Progress {
public:
  enum { ListTop = 2, ListRows = 20, TextTop = 1, TextRows = 22, TextColumn = 1 };

  WikiView( Canvas &canvas, const char *version );
  void open();
  void close();
  void drawTree( const TreeState &state );
  void drawPage( const ReaderState &state );
  void report( const char *text );
  void showMessage( const char *text );
  void showError( const Error &error );
  void clearStatus();

private:
  void titleBar( const char *left, const char *right );
  void drawTreeRow( int row, const TreeState &state, int item );
  void drawPageLine( int row, const char *line, const ReaderState &state, int lineNumber );
  void status( const char *text, unsigned char attr );

  Canvas &canvas_;
  const char *version_;
};

#endif
