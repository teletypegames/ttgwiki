#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wiki_info.h"
#include "../lib/core/ini.h"
#include "../lib/core/str.h"

WikiInfo::WikiInfo() : format( 0 ), generation( 0 ), languageCount( 0 ) {}

bool WikiInfo::read( const char *ini ) {
  IniScanner scanner( ini );
  IniLine line;
  char value[48];
  languageCount = 0;
  while ( scanner.next( line ) ) {
    line.copyValue( value, sizeof( value ) );
    if ( line.keyIs( "FORMAT" ) ) format = (unsigned)atoi( value );
    else if ( line.keyIs( "GENERATION" ) ) generation = strtoul( value, NULL, 10 );
    else if ( line.keyIs( "LANGUAGES" ) ) {
      for ( char *lang = strtok( value, ", " ); lang && languageCount < MaxLanguages; lang = strtok( NULL, ", " ) ) {
        copyStr( languages[languageCount], lang, sizeof( languages[0] ) );
        upcaseStr( languages[languageCount++] );
      }
    }
  }
  return format == Format && languageCount > 0;
}

bool WikiInfo::hasLanguage( const char *lang ) const {
  for ( int i = 0; i < languageCount; i++ ) {
    if ( cmpi( languages[i], lang ) == 0 ) return true;
  }
  return false;
}

const char *WikiInfo::nextLanguage( const char *lang ) const {
  if ( languageCount == 0 ) return lang;
  for ( int i = 0; i < languageCount; i++ ) {
    if ( cmpi( languages[i], lang ) == 0 ) return languages[( i + 1 ) % languageCount];
  }
  return languages[0];
}

void pageFileName( unsigned id, char out[13] ) {
  snprintf( out, 13, "P%07u.TXT", id );
}
