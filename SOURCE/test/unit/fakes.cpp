#include <stdio.h>
#include <string.h>

#include "fakes.h"
#include "../../lib/core/crc32.h"
#include "../../lib/core/error.h"
#include "../../lib/core/str.h"
#include "../../lib/net/byte_sink.h"
#include "../../lib/platform/screen.h"
#include "../../lib/ui/keys.h"

FakeRemote::FakeRemote() : connects( 0 ), downloads( 0 ), online( true ), connected( false ), count_( 0 ) {}

int FakeRemote::find( const char *path ) const {
  for ( int i = 0; i < count_; i++ ) {
    if ( strcmp( paths_[i], path ) == 0 ) return i;
  }
  return -1;
}

void FakeRemote::put( const char *path, const char *content ) {
  int i = find( path );
  if ( i < 0 ) {
    i = count_++;
    strcpy( paths_[i], path );
  }
  copyStr( contents_[i], content, ContentSize );
}

void FakeRemote::remove( const char *path ) {
  int i = find( path );
  if ( i < 0 ) return;
  count_--;
  strcpy( paths_[i], paths_[count_] );
  strcpy( contents_[i], contents_[count_] );
}

void FakeRemote::putInfo( unsigned long generation ) {
  char text[200];
  snprintf( text, sizeof( text ), "[wiki]\r\nformat=1\r\ngeneration=%lu\r\nlanguages=EN,HU\r\nwidth=78\r\n",
            generation );
  put( "WIKI.INI", text );
}

void FakeRemote::putPage( const char *lang, unsigned id, const char *title, const char *links, const char *lines ) {
  char path[40], text[ContentSize];
  snprintf( path, sizeof( path ), "%s/P%07u.TXT", lang, id );
  snprintf( text, sizeof( text ), "TITLE=%s\r\nROUTE=/x\r\nURL=https://wiki.example/x\r\n%s\r\n%s", title, links,
            lines );
  put( path, text );
}

void FakeRemote::putIndex( const char *lang, const char *lines ) {
  char path[40];
  snprintf( path, sizeof( path ), "%s/INDEX.TXT", lang );
  put( path, lines );
}

void FakeRemote::putManifest( const char *lang ) {
  char prefix[8], path[40], text[ContentSize];
  snprintf( prefix, sizeof( prefix ), "%s/", lang );
  text[0] = 0;
  for ( int i = 0; i < count_; i++ ) {
    if ( strncmp( paths_[i], prefix, strlen( prefix ) ) != 0 || strstr( paths_[i], "FILES.LST" ) ) continue;
    Crc32 crc;
    crc.update( contents_[i], strlen( contents_[i] ) );
    char line[80];
    snprintf( line, sizeof( line ), "%08lx %lu %s\r\n", (unsigned long)crc.value(),
              (unsigned long)strlen( contents_[i] ), paths_[i] + strlen( prefix ) );
    strcat( text, line );
  }
  snprintf( path, sizeof( path ), "%s/FILES.LST", lang );
  put( path, text );
}

bool FakeRemote::connect() {
  connects++;
  connected = online;
  return online;
}

void FakeRemote::disconnect() {
  connected = false;
}

void FakeRemote::shutdown() {
  connected = false;
}

RemoteWiki::Fetch FakeRemote::readText( const char *path, char *buf, unsigned size ) {
  if ( !connected ) return Failed;
  int i = find( path );
  if ( i < 0 ) return Missing;
  copyStr( buf, contents_[i], size );
  return Fetched;
}

bool FakeRemote::download( const char *path, ByteSink &sink, const char * ) {
  Error error;
  int i = find( path );
  if ( !connected || i < 0 ) return false;
  downloads++;
  return sink.write( contents_[i], strlen( contents_[i] ), error );
}

KeepScreen::KeepScreen( Screen &screen ) : screen_( screen ), kept_( false ) {
  for ( int r = 0; r < 25; r++ ) rows_[r][0] = 0;
}

unsigned KeepScreen::scriptEnded() {
  if ( !kept_ ) {
    for ( int r = 0; r < 25; r++ ) {
      int n = 0;
      for ( int c = 0; c < 80; c++ ) {
        rows_[r][c] = (char)( screen_.cell( r * 80 + c ) & 0xFF );
        attrs_[r][c] = (unsigned char)( screen_.cell( r * 80 + c ) >> 8 );
        if ( rows_[r][c] != ' ' ) n = c + 1;
      }
      rows_[r][n] = 0;
    }
    kept_ = true;
  }
  return KeyEscCode;
}

const char *KeepScreen::row( int n ) const {
  return rows_[n];
}

bool KeepScreen::contains( const char *text ) const {
  for ( int r = 0; r < 25; r++ ) {
    if ( strstr( rows_[r], text ) ) return true;
  }
  return false;
}

unsigned char KeepScreen::attr( int row, int column ) const {
  return attrs_[row][column];
}
