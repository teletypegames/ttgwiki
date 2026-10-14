#include "text_match.h"

bool containsFolded( const char *text, const char *query, CodePage cp ) {
  if ( *query == 0 ) return true;
  for ( const char *start = text; *start; start++ ) {
    const char *t = start;
    const char *q = query;
    while ( *t && *q && foldLetter( (unsigned char)*t, cp ) == foldLetter( (unsigned char)*q, cp ) ) {
      t++;
      q++;
    }
    if ( *q == 0 ) return true;
  }
  return false;
}
