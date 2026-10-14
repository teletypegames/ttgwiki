#include <stdio.h>
#include <string.h>

#include "wiki_view.h"
#include "../lib/core/error.h"
#include "../lib/core/str.h"
#include "../lib/ui/canvas.h"
#include "../lib/ui/list_cursor.h"
#include "../model/date.h"
#include "../model/page_index.h"
#include "../model/page_line.h"

enum { Cols = Canvas::Cols, InfoRow = 22, StatusRow = Canvas::Rows - 2, KeysRow = Canvas::Rows - 1 };

static const unsigned char AttrTitle = 0x1F;
static const unsigned char AttrHead = 0x70;
static const unsigned char AttrText = 0x07;
static const unsigned char AttrSelected = 0x1F;
static const unsigned char AttrDim = 0x08;
static const unsigned char AttrSearch = 0x0E;
static const unsigned char AttrStatus = 0x0F;
static const unsigned char AttrError = 0x4F;
static const unsigned char AttrLink = 0x09;
static const unsigned char AttrLinkSelected = 0x70;

static unsigned char styleAttr( char style ) {
  switch ( style ) {
    case StyleHeading1: return 0x0E;
    case StyleHeading2: return 0x0B;
    case StyleHeading3: return 0x0F;
    case StyleBold: return 0x0F;
    case StyleItalic: return 0x03;
    case StyleCode: return 0x0A;
    case StyleCodeBlock: return 0x17;
    case StyleQuote: return 0x06;
    case StyleRule: return 0x08;
    case StyleTableFrame: return 0x08;
    case StyleTableHeader: return 0x0F;
    default: return AttrText;
  }
}

WikiView::WikiView( Canvas &canvas, const char *version ) : canvas_( canvas ), version_( version ) {}

void WikiView::open() {
  canvas_.screen().begin();
  canvas_.screen().hideCursor();
}

void WikiView::close() {
  canvas_.screen().end();
}

void WikiView::titleBar( const char *left, const char *right ) {
  int width = strlen( right );
  canvas_.field( 0, 0, left, AttrTitle, Cols - width - 1 );
  canvas_.field( Cols - width - 1, 0, right, AttrTitle, width + 1 );
}

void WikiView::drawTreeRow( int row, const TreeState &state, int item ) {
  int y = ListTop + row;
  if ( item >= state.count ) {
    canvas_.field( 0, y, "", AttrText, Cols );
    return;
  }
  const PageEntry &page = state.index->at( state.visible[item] );
  bool selected = item == state.cursor->sel();
  unsigned char attr = selected ? AttrSelected : AttrText;
  int indent = state.query[0] ? 0 : page.depth * 2;
  if ( indent > 24 ) indent = 24;
  char line[Cols + 1];
  snprintf( line, sizeof( line ), " %*s%s", indent, "", page.title );
  canvas_.field( 0, y, line, attr, 66 );
  if ( cmpi( page.locale, state.lang ) != 0 ) {
    int pos = Canvas::at( y, 1 + indent + strlen( page.title ) + 1 );
    if ( pos < Canvas::at( y, 64 ) ) {
      char tag[8];
      snprintf( tag, sizeof( tag ), "[%s]", page.locale );
      canvas_.text( pos, tag, 6, selected ? AttrSelected : AttrDim );
    }
  }
  char date[11];
  formatDate( page.modified, date );
  canvas_.field( 66, y, date, selected ? AttrSelected : AttrDim, Cols - 66 );
}

void WikiView::drawTree( const TreeState &state ) {
  char left[Cols], right[32];
  snprintf( left, sizeof( left ), " TTGWIKI %s - Teletype Games Wiki", version_ );
  if ( state.generation ) snprintf( right, sizeof( right ), "%s  #%lu ", state.lang, state.generation );
  else snprintf( right, sizeof( right ), "%s ", state.lang );
  titleBar( left, right );
  canvas_.field( 0, 1, " Page", AttrHead, 66 );
  canvas_.field( 66, 1, "Updated", AttrHead, Cols - 66 );
  for ( int row = 0; row < ListRows; row++ ) drawTreeRow( row, state, state.cursor->top() + row );
  char info[Cols + 1];
  if ( state.query[0] ) {
    snprintf( info, sizeof( info ), " Search: %s_", state.query );
    canvas_.field( 0, InfoRow, info, AttrSearch, 66 );
  }
  else {
    const char *route = state.count ? state.index->at( state.visible[state.cursor->sel()] ).route : "";
    snprintf( info, sizeof( info ), " %s", route );
    canvas_.field( 0, InfoRow, info, AttrDim, 66 );
  }
  snprintf( info, sizeof( info ), "%d/%d", state.count ? state.cursor->sel() + 1 : 0, state.count );
  canvas_.field( 66, InfoRow, info, AttrDim, Cols - 66 );
  canvas_.field( 0, KeysRow, "", 0x70, Cols );
  canvas_.footer( Canvas::at( KeysRow, 0 ),
                  " ~Enter~ Read  ~Type~ Search  ~F2~ Language  ~F5~ Refresh  ~F6~ Download all  ~Esc~ Quit" );
  canvas_.flush();
}

void WikiView::drawPageLine( int row, const char *line, const ReaderState &state, int lineNumber ) {
  int y = TextTop + row;
  int pos = Canvas::at( y, 0 );
  pos = canvas_.put( pos, ' ', AttrText );
  LineReader reader( line );
  LineSegment segment;
  bool selectedLine = lineNumber == state.selectedLine;
  while ( reader.next( segment ) ) {
    for ( int i = 0; i < segment.length && pos < Canvas::at( y, Cols ); i++ ) {
      int column = segment.column + i;
      unsigned char attr = styleAttr( segment.style );
      if ( segment.link ) {
        bool selected =
            selectedLine && column >= state.selectedColumn && column < state.selectedColumn + state.selectedLength;
        attr = selected ? AttrLinkSelected : ( attr & 0xF0 ) | AttrLink;
      }
      pos = canvas_.put( pos, segment.text[i], attr );
    }
  }
  canvas_.fillTo( pos, Canvas::at( y + 1, 0 ), styleAttr( reader.style() ) & 0xF0 );
}

void WikiView::drawPage( const ReaderState &state ) {
  char right[32];
  int last = state.top + TextRows < state.lineCount ? state.top + TextRows : state.lineCount;
  snprintf( right, sizeof( right ), "%s  %d-%d/%d ", state.lang, state.lineCount ? state.top + 1 : 0, last,
            state.lineCount );
  char left[Cols];
  snprintf( left, sizeof( left ), " %s", state.title );
  titleBar( left, right );
  for ( int row = 0; row < TextRows; row++ ) {
    if ( row < state.shown ) drawPageLine( row, state.lines[row], state, state.top + row );
    else canvas_.field( 0, TextTop + row, "", AttrText, Cols );
  }
  canvas_.field( 0, KeysRow, "", 0x70, Cols );
  canvas_.footer( Canvas::at( KeysRow, 0 ),
                  " ~Tab~ Next link  ~Enter~ Follow  ~Bksp~ Back  ~F2~ Language  ~Esc~ Contents" );
  canvas_.flush();
}

void WikiView::status( const char *text, unsigned char attr ) {
  canvas_.field( 0, StatusRow, text, attr, Cols );
  canvas_.flush();
}

void WikiView::report( const char *text ) {
  char line[Cols + 1];
  snprintf( line, sizeof( line ), " %.78s", text );
  status( line, AttrStatus );
}

void WikiView::showMessage( const char *text ) {
  report( text );
}

void WikiView::showError( const Error &error ) {
  char line[Cols + 1];
  snprintf( line, sizeof( line ), " Error: %.70s", error.isSet() ? error.text() : "failed" );
  status( line, AttrError );
}

void WikiView::clearStatus() {
  status( "", AttrStatus );
}
