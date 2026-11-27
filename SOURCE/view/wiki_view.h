/*
   The screens of TTGWIKI: the contents (the page tree, or the pages a search found) and
   the reader of a page. Both have the title on the top line, the status line and the
   keys at the bottom. As the Progress of the operations it shows what they do in the
   status line.
*/

#ifndef TTGWIKI_VIEW_WIKI_VIEW_H
#define TTGWIKI_VIEW_WIKI_VIEW_H

#include "../lib/core/progress.h"
#include "../lib/platform/screen.h"

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
  const char *query; /* the title search, "" for none */
  const char *text;  /* the text search */
  int textMode;      /* NoText, TypingText or TextFound */
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

enum TextMode { NoText, TypingText, TextFound };

class WikiView : public Progress {
public:
  enum { ListTop = 2, TextTop = 1, TextColumn = 1, MaxTextRows = Screen::MaxRows - 3 };

  WikiView( Canvas &canvas, const char *version );
  /* Takes the screen over: the rows of the lists are known from here on. */
  void open();
  void close();
  /* The rows of the contents' list and of the page text (20 and 22 on 25 rows). */
  int listRows() const;
  int textRows() const;
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
  int infoRow() const;
  int statusRow() const;
  int keysRow() const;

  Canvas &canvas_;
  const char *version_;
};

#endif
