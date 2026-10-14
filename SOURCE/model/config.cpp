#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "../lib/core/ini.h"
#include "../lib/core/str.h"

Config::Config() : port( 2121 ), codePage( CodePage437 ) {
  strcpy( server, "ftp.teletypegames.org" );
  strcpy( user, "anonymous" );
  strcpy( password, "ttgwiki@" );
  strcpy( root, "/" );
  strcpy( lang, "EN" );
  cache[0] = 0;
}

static unsigned numberOf( const IniLine &line ) {
  char value[8];
  line.copyValue( value, sizeof( value ) );
  return (unsigned)atoi( value );
}

bool Config::read( const char *ini ) {
  IniScanner scanner( ini );
  IniLine line;
  bool ok = true;
  while ( scanner.next( line ) ) {
    if ( line.keyIs( "SERVER" ) ) line.copyValue( server, sizeof( server ) );
    else if ( line.keyIs( "PORT" ) ) port = numberOf( line );
    else if ( line.keyIs( "USER" ) ) line.copyValue( user, sizeof( user ) );
    else if ( line.keyIs( "PASSWORD" ) ) line.copyValue( password, sizeof( password ) );
    else if ( line.keyIs( "PATH" ) ) line.copyValue( root, sizeof( root ) );
    else if ( line.keyIs( "LANG" ) ) line.copyValue( lang, sizeof( lang ) );
    else if ( line.keyIs( "CACHE" ) ) line.copyValue( cache, sizeof( cache ) );
    else if ( line.keyIs( "CODEPAGE" ) ) ok = codePageOf( numberOf( line ), codePage ) && ok;
  }
  upcaseStr( lang );
  int len = strlen( cache );
  if ( len > 3 && cache[len - 1] == '\\' ) cache[len - 1] = 0;
  return ok;
}
