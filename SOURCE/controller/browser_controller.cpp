#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "browser_controller.h"
#include "../lib/core/error.h"
#include "../lib/core/str.h"
#include "../lib/platform/idle.h"
#include "../lib/platform/keyboard.h"
#include "../lib/ui/keys.h"
#include "../model/config.h"
#include "../model/page_line.h"
#include "../model/text_match.h"
#include "../model/word_query.h"
#include "../service/reader_service.h"
#include "../service/search_service.h"
#include "../service/sync_service.h"

enum { ShiftTabCode = 0x0F00 };

BrowserController::BrowserController( SyncService &sync, ReaderService &reader, SearchService &search, PageIndex &index,
                                      WikiView &view, Keyboard &keyboard, Idle &idle, Config &config, Error &error )
  : sync_( sync ), reader_( reader ), search_( search ), index_( index ), view_( view ), keyboard_( keyboard ),
    idle_( idle ), config_( config ), error_( error ), mode_( Contents ), visibleCount_( 0 ), textMode_( NoText ),
    textFound_( false ), top_( 0 ), textRows_( 0 ) {
  query_[0] = text_[0] = 0;
  clearSelection();
  for ( int i = 0; i < WikiView::MaxTextRows; i++ ) linePointers_[i] = lines_[i];
}

unsigned BrowserController::waitKey() {
  while ( !keyboard_.ready() ) idle_.idle();
  return keyboard_.read();
}

void BrowserController::run() {
  keyboard_.begin();
  view_.open();
  cursor_.setRows( view_.listRows() );
  textRows_ = view_.textRows();
  sync_.loadLocal( config_.lang );
  rebuildVisible( 0 );
  drawContents();
  refresh();
  for ( ;; ) {
    unsigned key = waitKey();
    bool go = mode_ == Contents ? contentsKey( key ) : readerKey( key );
    if ( !go ) break;
  }
  reader_.close();
  view_.close();
  keyboard_.end();
}

/* ---- the contents ---- */

void BrowserController::rebuildVisible( unsigned keepId ) {
  visibleCount_ = 0;
  int keep = 0;
  for ( int i = 0; i < index_.count(); i++ ) {
    const PageEntry &page = index_.at( i );
    if ( textFound_ && !search_.found( page.id ) ) continue;
    if ( query_[0] && !containsFolded( page.title, query_, config_.codePage ) &&
         !containsFolded( page.route, query_, config_.codePage ) ) {
      continue;
    }
    if ( page.id == keepId ) keep = visibleCount_;
    visible_[visibleCount_++] = i;
  }
  cursor_.setCount( visibleCount_ );
  cursor_.place( keep, cursor_.top() );
  cursor_.clamp();
}

unsigned BrowserController::selectedId() const {
  return visibleCount_ ? index_.at( visible_[cursor_.sel()] ).id : 0;
}

void BrowserController::drawContents() {
  TreeState state = { &index_,   visible_,     visibleCount_,          &cursor_, query_, text_,
                      textMode_, config_.lang, sync_.info().generation };
  view_.drawTree( state );
}

void BrowserController::refresh() {
  unsigned keep = mode_ == Reading ? reader_.id() : selectedId();
  int top = top_;
  view_.report( "Looking for changes..." );
  SyncService::Result result = sync_.refresh( config_.lang );
  rebuildVisible( keep );
  if ( mode_ == Reading ) {
    if ( !openPage( keep, top ) ) toContents();
  }
  else drawContents();
  char msg[80];
  if ( result == SyncService::Failed ) {
    view_.showError( error_ );
    error_.clear();
    return;
  }
  snprintf( msg, sizeof( msg ), "%d pages, export #%lu%s", index_.count(), sync_.info().generation,
            result == SyncService::Updated ? " (new)" : "" );
  view_.showMessage( msg );
}

bool BrowserController::contentsKey( unsigned key ) {
  unsigned char ch = keyAscii( key ), scan = keyScan( key );
  if ( isExtendedKey( key ) ) {
    if ( scan == ScanF3 ) {
      textMode_ = TypingText;
      query_[0] = 0;
      rebuildVisible( selectedId() );
      drawContents();
    }
    else if ( scan == ScanF2 ) switchLanguage();
    else if ( scan == ScanF5 ) refresh();
    else if ( scan == ScanF6 ) downloadAll();
    else moveSelection( scan );
    return true;
  }
  if ( textMode_ != NoText && textKey( ch ) ) return true;
  if ( ch == KeyEsc ) {
    if ( query_[0] == 0 ) return false;
    unsigned keep = selectedId();
    query_[0] = 0;
    rebuildVisible( keep );
    drawContents();
    return true;
  }
  if ( ch == KeyEnter ) {
    if ( visibleCount_ ) {
      history_.clear();
      openPage( selectedId(), 0 );
    }
    return true;
  }
  typeQuery( ch );
  return true;
}

void BrowserController::moveSelection( unsigned char scan ) {
  int sel = cursor_.sel();
  int page = cursor_.rows() - 1;
  if ( scan == ScanUp ) sel--;
  else if ( scan == ScanDown ) sel++;
  else if ( scan == ScanPgUp ) sel -= page;
  else if ( scan == ScanPgDn ) sel += page;
  else if ( scan == ScanHome ) sel = 0;
  else if ( scan == ScanEnd ) sel = visibleCount_ - 1;
  else return;
  if ( sel < 0 ) sel = 0;
  if ( sel >= visibleCount_ ) sel = visibleCount_ - 1;
  if ( sel < 0 ) return;
  cursor_.select( sel );
  drawContents();
}

void BrowserController::typeQuery( unsigned char ch ) {
  int len = strlen( query_ );
  if ( ch == KeyBackspace ) {
    if ( len == 0 ) return;
    query_[len - 1] = 0;
  }
  else if ( ch >= ' ' && len < QuerySize - 1 && !( ch == ' ' && len == 0 ) ) {
    query_[len] = (char)ch;
    query_[len + 1] = 0;
  }
  else return;
  rebuildVisible( 0 );
  cursor_.place( 0, 0 );
  drawContents();
}

/* The keys of the text search; false for the ones it leaves to the contents. */
bool BrowserController::textKey( unsigned char ch ) {
  if ( ch == KeyEsc ) {
    leaveText();
    return true;
  }
  if ( ch == KeyEnter ) {
    if ( textMode_ == TextFound ) return false;
    searchText();
    return true;
  }
  int len = strlen( text_ );
  if ( ch == KeyBackspace ) {
    if ( len ) text_[len - 1] = 0;
  }
  else if ( ch >= ' ' && len < QuerySize - 1 ) {
    text_[len] = (char)ch;
    text_[len + 1] = 0;
  }
  else return true;
  textMode_ = TypingText;
  drawContents();
  return true;
}

void BrowserController::searchText() {
  WordQuery query;
  query.parse( text_, config_.codePage );
  view_.report( "Searching the text..." );
  SearchService::Result result = search_.search( config_.lang, query );
  if ( result == SearchService::NoWords ) {
    view_.showMessage( "Type words of 3 letters or more" );
    return;
  }
  if ( result == SearchService::NoIndex ) {
    view_.showMessage( "This export cannot be searched: the server is older" );
    return;
  }
  if ( result == SearchService::Failed ) {
    view_.showError( error_ );
    error_.clear();
    return;
  }
  textMode_ = TextFound;
  textFound_ = true;
  rebuildVisible( 0 );
  cursor_.place( 0, 0 );
  drawContents();
  char msg[80];
  if ( visibleCount_ == 0 ) snprintf( msg, sizeof( msg ), "No page has every word" );
  else snprintf( msg, sizeof( msg ), "%d pages have every word", visibleCount_ );
  view_.showMessage( msg );
}

void BrowserController::leaveText() {
  unsigned keep = selectedId();
  textMode_ = NoText;
  textFound_ = false;
  search_.clear();
  rebuildVisible( keep );
  cursor_.scrollToSelection();
  drawContents();
  view_.clearStatus();
}

void BrowserController::switchLanguage() {
  const WikiInfo &info = sync_.info();
  char next[4];
  if ( info.languageCount > 1 ) copyStr( next, info.nextLanguage( config_.lang ), sizeof( next ) );
  else copyStr( next, cmpi( config_.lang, "EN" ) == 0 ? "HU" : "EN", sizeof( next ) );
  if ( cmpi( next, config_.lang ) == 0 ) {
    view_.showMessage( "The wiki has one language only" );
    return;
  }
  unsigned keep = mode_ == Reading ? reader_.id() : selectedId();
  textMode_ = NoText;
  textFound_ = false;
  search_.clear();
  copyStr( config_.lang, next, sizeof( config_.lang ) );
  sync_.loadLocal( config_.lang );
  view_.report( "Changing the language..." );
  SyncService::Result result = sync_.refresh( config_.lang );
  rebuildVisible( keep );
  if ( mode_ == Reading ) {
    if ( index_.find( keep ) >= 0 && openPage( keep, top_ ) ) return;
    toContents();
  }
  else drawContents();
  if ( result == SyncService::Failed ) {
    view_.showError( error_ );
    error_.clear();
  }
  else {
    char msg[40];
    snprintf( msg, sizeof( msg ), "Language: %s", config_.lang );
    view_.showMessage( msg );
  }
}

void BrowserController::downloadAll() {
  view_.report( "Downloading the pages..." );
  int done = sync_.downloadAll( config_.lang );
  char msg[80];
  if ( done < 0 ) {
    view_.showError( error_ );
    error_.clear();
    return;
  }
  if ( done == 0 ) snprintf( msg, sizeof( msg ), "Every %s page is on the disk already", config_.lang );
  else snprintf( msg, sizeof( msg ), "Downloaded %d pages: every %s page can be read offline", done, config_.lang );
  view_.showMessage( msg );
}

/* ---- the reader ---- */

void BrowserController::clearSelection() {
  selLine_ = -1;
  selColumn_ = selLength_ = selLink_ = 0;
}

bool BrowserController::openPage( unsigned id, int top ) {
  unsigned previous = mode_ == Reading ? reader_.id() : 0;
  view_.report( "Opening the page..." );
  bool ok = reader_.open( id );
  rebuildVisible( id );
  if ( !ok ) {
    char why[Error::Size];
    copyStr( why, error_.text(), sizeof( why ) );
    if ( previous ) reader_.open( previous );
    error_.set( "%s", why );
    view_.showError( error_ );
    error_.clear();
    return false;
  }
  mode_ = Reading;
  top_ = top;
  clampTop();
  clearSelection();
  drawPage();
  showPageMessage();
  return true;
}

void BrowserController::showPageMessage() {
  if ( reader_.truncated() ) {
    char msg[160];
    snprintf( msg, sizeof( msg ), "The page is too long, the end is left out: %s", reader_.url() );
    view_.showMessage( msg );
  }
  else view_.clearStatus();
}

void BrowserController::drawPage() {
  int shown = 0;
  for ( int row = 0; row < textRows_ && top_ + row < reader_.lineCount(); row++ ) {
    if ( !reader_.readLine( top_ + row, lines_[row], LineSize ) ) break;
    shown++;
  }
  ReaderState state = { reader_.title(), reader_.lineCount(), linePointers_, shown,       top_,
                        selLine_,        selColumn_,          selLength_,    config_.lang };
  view_.drawPage( state );
}

void BrowserController::clampTop() {
  int max = reader_.lineCount() - textRows_;
  if ( top_ > max ) top_ = max;
  if ( top_ < 0 ) top_ = 0;
}

bool BrowserController::readerKey( unsigned key ) {
  unsigned char ch = keyAscii( key ), scan = keyScan( key );
  if ( key == ShiftTabCode ) previousLink();
  else if ( isExtendedKey( key ) ) {
    if ( scan == ScanF2 ) switchLanguage();
    else if ( scan == ScanF5 ) refresh();
    else if ( scan == ScanLeft ) back();
    else scroll( scan );
  }
  else if ( ch == KeyTab ) nextLink();
  else if ( ch == KeyEnter ) followLink();
  else if ( ch == KeyBackspace ) back();
  else if ( ch == KeyEsc ) toContents();
  return true;
}

void BrowserController::scroll( unsigned char scan ) {
  int page = textRows_ - 1;
  int top = top_;
  if ( scan == ScanUp ) top_--;
  else if ( scan == ScanDown ) top_++;
  else if ( scan == ScanPgUp ) top_ -= page;
  else if ( scan == ScanPgDn ) top_ += page;
  else if ( scan == ScanHome ) top_ = 0;
  else if ( scan == ScanEnd ) top_ = reader_.lineCount();
  else return;
  clampTop();
  if ( top_ == top ) return;
  if ( selLine_ >= 0 && ( selLine_ < top_ || selLine_ >= top_ + textRows_ ) ) {
    clearSelection();
    view_.clearStatus();
  }
  drawPage();
}

void BrowserController::showLine( int line ) {
  if ( line >= top_ && line < top_ + textRows_ ) return;
  top_ = line - textRows_ / 2;
  clampTop();
}

void BrowserController::selectLink( int line, const LinkSpot &spot ) {
  selLine_ = line;
  selColumn_ = spot.column;
  selLength_ = spot.length;
  selLink_ = spot.link;
  showLine( line );
  drawPage();
  showSelectedTarget();
}

void BrowserController::nextLink() {
  char line[LineSize];
  LinkSpot spot;
  int lines = reader_.lineCount();
  int start = selLine_ >= 0 ? selLine_ : top_;
  int column = selLine_ >= 0 ? selColumn_ + 1 : 0;
  for ( int pass = 0; pass < 2; pass++ ) {
    int from = pass == 0 ? start : 0;
    int to = pass == 0 ? lines : start + 1;
    for ( int n = from; n < to; n++ ) {
      if ( reader_.hasLinks( n ) && reader_.readLine( n, line, sizeof( line ) ) &&
           findLinkAfter( line, column, spot ) ) {
        selectLink( n, spot );
        return;
      }
      column = 0;
    }
  }
  view_.showMessage( "There are no links on this page" );
}

void BrowserController::previousLink() {
  char line[LineSize];
  LinkSpot spot;
  int lines = reader_.lineCount();
  if ( lines == 0 ) return;
  int start = selLine_ >= 0 ? selLine_ : top_ + textRows_ - 1;
  if ( start >= lines ) start = lines - 1;
  int column = selLine_ >= 0 ? selColumn_ : LineSize;
  for ( int pass = 0; pass < 2; pass++ ) {
    int from = pass == 0 ? start : lines - 1;
    int to = pass == 0 ? -1 : start - 1;
    for ( int n = from; n > to; n-- ) {
      if ( reader_.hasLinks( n ) && reader_.readLine( n, line, sizeof( line ) ) &&
           findLinkBefore( line, column, spot ) ) {
        selectLink( n, spot );
        return;
      }
      column = LineSize;
    }
  }
  view_.showMessage( "There are no links on this page" );
}

void BrowserController::showSelectedTarget() {
  char target[160], msg[200];
  if ( selLine_ < 0 || !reader_.linkTarget( selLink_, target, sizeof( target ) ) ) {
    view_.clearStatus();
    return;
  }
  if ( target[0] == '#' ) {
    int i = index_.find( (unsigned)atoi( target + 1 ) );
    snprintf( msg, sizeof( msg ), "\x1A %s", i >= 0 ? index_.at( i ).title : "a page" );
  }
  else snprintf( msg, sizeof( msg ), "Web: %s", target );
  view_.showMessage( msg );
}

void BrowserController::followLink() {
  char target[160];
  if ( selLine_ < 0 ) {
    view_.showMessage( "Select a link with Tab first" );
    return;
  }
  if ( !reader_.linkTarget( selLink_, target, sizeof( target ) ) ) return;
  if ( target[0] != '#' ) {
    char msg[200];
    snprintf( msg, sizeof( msg ), "A web page, open it in a browser: %s", target );
    view_.showMessage( msg );
    return;
  }
  Visit visit = { reader_.id(), top_, selLine_, selColumn_ };
  history_.push( visit );
  Visit dropped;
  if ( !openPage( (unsigned)atoi( target + 1 ), 0 ) ) history_.pop( dropped );
}

void BrowserController::back() {
  Visit visit;
  if ( !history_.pop( visit ) ) {
    toContents();
    return;
  }
  if ( !openPage( visit.id, visit.top ) ) return;
  if ( visit.linkLine < 0 ) return;
  char line[LineSize];
  LinkSpot spot;
  if ( reader_.readLine( visit.linkLine, line, sizeof( line ) ) && findLinkAfter( line, visit.linkColumn, spot ) ) {
    selectLink( visit.linkLine, spot );
  }
}

void BrowserController::toContents() {
  unsigned id = reader_.id();
  mode_ = Contents;
  history_.clear();
  rebuildVisible( id );
  cursor_.scrollToSelection();
  drawContents();
  view_.clearStatus();
}
