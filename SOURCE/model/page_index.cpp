#include <stdlib.h>
#include <string.h>

#include "page_index.h"
#include "../lib/core/str.h"

PageIndex::PageIndex( char *text ) : text_( text ), count_( 0 ) {
  text_[0] = 0;
}

char *PageIndex::text() {
  return text_;
}

void PageIndex::clear() {
  text_[0] = 0;
  count_ = 0;
}

int PageIndex::count() const {
  return count_;
}

const PageEntry &PageIndex::at( int i ) const {
  return pages_[i];
}

/* The fields of a line, split at the tabs in place. */
static int splitFields( char *line, char *fields[], int max ) {
  int n = 0;
  fields[n++] = line;
  for ( char *p = line; *p && n < max; p++ ) {
    if ( *p == '\t' ) {
      *p = 0;
      fields[n++] = p + 1;
    }
  }
  return n;
}

bool PageIndex::parseLine( char *line, PageEntry &entry, CodePage cp ) {
  char *fields[7];
  if ( splitFields( line, fields, 7 ) < 7 ) return false;
  entry.id = (unsigned)atoi( fields[0] );
  entry.parent = (unsigned)atoi( fields[1] );
  entry.depth = (unsigned char)atoi( fields[2] );
  entry.modified = strtoul( fields[3], NULL, 10 );
  entry.locale = fields[4];
  entry.route = fields[5];
  utf8ToCodePage( fields[6], cp );
  entry.title = fields[6];
  return entry.id != 0;
}

int PageIndex::parse( CodePage cp ) {
  count_ = 0;
  char *line = text_;
  while ( *line && count_ < MaxPages ) {
    char *end = line;
    while ( *end && *end != '\r' && *end != '\n' ) end++;
    char *next = end;
    while ( *next == '\r' || *next == '\n' ) next++;
    *end = 0;
    if ( parseLine( line, pages_[count_], cp ) ) count_++;
    line = next;
  }
  return count_;
}

int PageIndex::find( unsigned id ) const {
  for ( int i = 0; i < count_; i++ ) {
    if ( pages_[i].id == id ) return i;
  }
  return -1;
}

int PageIndex::findRoute( const char *route ) const {
  for ( int i = 0; i < count_; i++ ) {
    if ( strcmp( pages_[i].route, route ) == 0 ) return i;
  }
  return -1;
}
