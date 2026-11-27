#include <stdio.h>
#include <string.h>

#include "word_query.h"

static const char WordFilePrefix[] = "WORDS_";

WordQuery::WordQuery() : count_( 0 ) {}

static bool isWordChar( unsigned char c ) {
  return ( c >= 'A' && c <= 'Z' ) || ( c >= '0' && c <= '9' );
}

int WordQuery::parse( const char *text, CodePage cp ) {
  count_ = 0;
  int len = 0;
  for ( const char *p = text;; p++ ) {
    unsigned char c = *p ? foldLetter( (unsigned char)*p, cp ) : 0;
    if ( isWordChar( c ) ) {
      if ( len < MaxLength && count_ < MaxWords ) words_[count_][len++] = (char)c;
      continue;
    }
    if ( len >= MinLength && count_ < MaxWords ) words_[count_++][len] = 0;
    len = 0;
    if ( c == 0 ) break;
  }
  return count_;
}

int WordQuery::count() const {
  return count_;
}

const char *WordQuery::word( int i ) const {
  return words_[i];
}

void wordFileName( const char *word, char out[13] ) {
  snprintf( out, 13, "%s%c.TXT", WordFilePrefix, word[0] );
}

bool isWordFile( const char *name ) {
  return strncmp( name, WordFilePrefix, sizeof( WordFilePrefix ) - 1 ) == 0;
}

const char *wordLineIds( const char *line, const char *word ) {
  while ( *word ) {
    if ( *line++ != *word++ ) return NULL;
  }
  while ( isWordChar( (unsigned char)*line ) ) line++;
  return *line == ' ' ? line + 1 : NULL;
}

bool nextWordId( const char *&pos, unsigned &id ) {
  if ( *pos < '0' || *pos > '9' ) return false;
  id = 0;
  while ( *pos >= '0' && *pos <= '9' ) id = id * 10 + ( *pos++ - '0' );
  if ( *pos == ',' ) pos++;
  return true;
}
