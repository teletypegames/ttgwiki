#include <stdlib.h>

#include "page_line.h"

enum { CodeStyle = 1, CodeLink = 2, CodeLinkNumberEnd = 3, CodeLinkEnd = 4 };

LineReader::LineReader( const char *line ) : pos_( line ), column_( 0 ), style_( StyleNormal ), link_( 0 ) {}

void LineReader::readControls() {
  for ( ;; ) {
    if ( *pos_ == CodeStyle && pos_[1] ) {
      style_ = pos_[1];
      pos_ += 2;
    }
    else if ( *pos_ == CodeLink ) {
      link_ = atoi( pos_ + 1 );
      while ( *pos_ && *pos_ != CodeLinkNumberEnd ) pos_++;
      if ( *pos_ ) pos_++;
    }
    else if ( *pos_ == CodeLinkEnd ) {
      link_ = 0;
      pos_++;
    }
    else return;
  }
}

static bool isControl( char c ) {
  return c >= CodeStyle && c <= CodeLinkEnd;
}

bool LineReader::next( LineSegment &segment ) {
  readControls();
  if ( *pos_ == 0 ) return false;
  segment.text = pos_;
  segment.column = column_;
  segment.style = style_;
  segment.link = link_;
  while ( *pos_ && !isControl( *pos_ ) ) pos_++;
  segment.length = (int)( pos_ - segment.text );
  column_ += segment.length;
  return true;
}

/* The links of a line, each as one spot even when its style changes inside. */
class LinkScanner {
public:
  explicit LinkScanner( const char *line ) : reader_( line ), pendingValid_( false ) {}

  bool next( LinkSpot &spot ) {
    LineSegment segment;
    bool open = false;
    for ( ;; ) {
      if ( pendingValid_ ) {
        segment = pending_;
        pendingValid_ = false;
      }
      else if ( !reader_.next( segment ) ) return open;
      if ( open ) {
        if ( segment.link == spot.link && segment.column == spot.column + spot.length ) {
          spot.length += segment.length;
          continue;
        }
        pending_ = segment;
        pendingValid_ = true;
        return true;
      }
      if ( segment.link ) {
        spot.column = segment.column;
        spot.length = segment.length;
        spot.link = segment.link;
        open = true;
      }
    }
  }

private:
  LineReader reader_;
  LineSegment pending_;
  bool pendingValid_;
};

bool findLinkAfter( const char *line, int fromColumn, LinkSpot &spot ) {
  LinkScanner scanner( line );
  while ( scanner.next( spot ) ) {
    if ( spot.column >= fromColumn ) return true;
  }
  return false;
}

bool findLinkBefore( const char *line, int beforeColumn, LinkSpot &spot ) {
  LinkScanner scanner( line );
  LinkSpot candidate;
  bool found = false;
  while ( scanner.next( candidate ) && candidate.column < beforeColumn ) {
    spot = candidate;
    found = true;
  }
  return found;
}

void plainLine( const char *line, char *out, int size ) {
  LineReader reader( line );
  LineSegment segment;
  int n = 0;
  while ( reader.next( segment ) ) {
    for ( int i = 0; i < segment.length && n < size - 1; i++ ) out[n++] = segment.text[i];
  }
  out[n] = 0;
}
