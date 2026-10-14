#include <string.h>

#include "../../lib/testing/host/minitest.h"
#include "../../model/config.h"
#include "../../model/date.h"
#include "../../model/history.h"
#include "../../model/manifest.h"
#include "../../model/page_index.h"
#include "../../model/page_line.h"
#include "../../model/text_match.h"
#include "../../model/wiki_info.h"

TEST( configReadsTheSettings ) {
  Config config;
  CHECK_STR( config.server, "ftp.teletypegames.org" );
  CHECK_INT( config.port, 2121 );
  CHECK( config.read( "SERVER=10.0.2.2\r\nPORT=21\r\nLANG=hu\r\nCODEPAGE=852\r\nCACHE=D:\\WIKI\\\r\n" ) );
  CHECK_STR( config.server, "10.0.2.2" );
  CHECK_INT( config.port, 21 );
  CHECK_STR( config.lang, "HU" );
  CHECK( config.codePage == CodePage852 );
  CHECK_STR( config.cache, "D:\\WIKI" );
  CHECK( !config.read( "CODEPAGE=850\r\n" ) );
}

TEST( wikiInfoReadsTheLanguages ) {
  WikiInfo info;
  CHECK( info.read( "[wiki]\r\nformat=1\r\ngeneration=42\r\nlanguages=EN,HU\r\n" ) );
  CHECK_INT( (long)info.generation, 42 );
  CHECK_INT( info.languageCount, 2 );
  CHECK( info.hasLanguage( "hu" ) );
  CHECK_STR( info.nextLanguage( "EN" ), "HU" );
  CHECK_STR( info.nextLanguage( "HU" ), "EN" );
  CHECK( !info.read( "[wiki]\r\nformat=2\r\nlanguages=EN\r\n" ) );
  char name[13];
  pageFileName( 123, name );
  CHECK_STR( name, "P0000123.TXT" );
}

TEST( manifestFindsAndComparesEntries ) {
  Manifest a( "0000000a 10 INDEX.TXT\r\n0000000b 20 P0000001.TXT\r\n" );
  Manifest b( "0000000a 10 INDEX.TXT\r\n0000000c 20 P0000001.TXT\r\n" );
  ManifestEntry entry;
  CHECK( a.find( "p0000001.txt", entry ) );
  CHECK_INT( (long)entry.crc, 11 );
  CHECK_INT( (long)entry.size, 20 );
  CHECK( !a.sameIn( b, entry ) );
  CHECK( a.find( "INDEX.TXT", entry ) && a.sameIn( b, entry ) );
  int n = 0;
  const char *pos = a.first();
  while ( a.next( pos, entry ) ) n++;
  CHECK_INT( n, 2 );
  CHECK( !Manifest( NULL ).find( "INDEX.TXT", entry ) );
}

TEST( pageIndexParsesTheTree ) {
  static char text[PageIndex::TextSize];
  PageIndex index( text );
  strcpy( text, "26\t0\t0\t1789548052\ten\t/\tHome\r\n"
                "16\t26\t1\t1791920030\thu\t/dos/dosinst\tF\xC5\x91oldal\r\nbroken line\r\n" );
  CHECK_INT( index.parse( CodePage852 ), 2 );
  CHECK_INT( index.at( 1 ).id, 16 );
  CHECK_INT( index.at( 1 ).parent, 26 );
  CHECK_INT( index.at( 1 ).depth, 1 );
  CHECK_STR( index.at( 1 ).locale, "hu" );
  CHECK_STR( index.at( 1 ).route, "/dos/dosinst" );
  CHECK_STR( index.at( 1 ).title, "F\x8Boldal" );
  CHECK_INT( index.find( 26 ), 0 );
  CHECK_INT( index.find( 99 ), -1 );
  CHECK_INT( index.findRoute( "/dos/dosinst" ), 1 );
}

TEST( lineReaderSplitsStylesAndLinks ) {
  LineReader reader( "a \x01"
                     "bbold\x01"
                     "0 \x02"
                     "12\x03link\x04 end" );
  LineSegment s;
  CHECK( reader.next( s ) && s.length == 2 && s.style == StyleNormal && s.column == 0 );
  CHECK( reader.next( s ) && s.length == 4 && s.style == StyleBold && s.column == 2 );
  CHECK( reader.next( s ) && s.length == 1 && s.link == 0 );
  CHECK( reader.next( s ) && s.length == 4 && s.link == 12 && s.column == 7 );
  CHECK( reader.next( s ) && s.link == 0 && s.column == 11 );
  CHECK( !reader.next( s ) );
}

TEST( linksAreFoundForwardAndBackward ) {
  const char *line = "x \x02"
                     "1\x03"
                     "one\x01"
                     "bB\x04\x01"
                     "0 \x02"
                     "2\x03two\x04";
  LinkSpot spot;
  CHECK( findLinkAfter( line, 0, spot ) );
  CHECK_INT( spot.column, 2 );
  CHECK_INT( spot.length, 4 );
  CHECK_INT( spot.link, 1 );
  CHECK( findLinkAfter( line, 3, spot ) && spot.link == 2 && spot.column == 7 );
  CHECK( !findLinkAfter( line, 8, spot ) );
  CHECK( findLinkBefore( line, 7, spot ) && spot.link == 1 );
  CHECK( !findLinkBefore( line, 2, spot ) );
  char plain[40];
  plainLine( line, plain, sizeof( plain ) );
  CHECK_STR( plain, "x oneB two" );
}

TEST( datesAreUtc ) {
  char date[11];
  formatDate( 0, date );
  CHECK_STR( date, "1970-01-01" );
  formatDate( 1791920030UL, date );
  CHECK_STR( date, "2026-10-13" );
  formatDate( 951782400UL, date );
  CHECK_STR( date, "2000-02-29" );
}

TEST( searchIgnoresCaseAndAccents ) {
  CHECK( containsFolded( "Fejleszt\x82s", "FEJLESZTES", CodePage437 ) );
  CHECK( containsFolded( "Kezd\x8Blap", "kezdo", CodePage852 ) );
  CHECK( !containsFolded( "GMENU", "dos", CodePage437 ) );
  CHECK( containsFolded( "x", "", CodePage437 ) );
}

TEST( historyForgetsTheOldest ) {
  History history;
  for ( unsigned i = 1; i <= History::Size + 2; i++ ) {
    Visit visit = { i, 0, -1, 0 };
    history.push( visit );
  }
  CHECK_INT( history.count(), History::Size );
  Visit visit;
  CHECK( history.pop( visit ) && visit.id == History::Size + 2 );
  while ( history.pop( visit ) ) {
  }
  CHECK_INT( visit.id, 3 );
}
