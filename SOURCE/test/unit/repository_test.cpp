#include <string.h>

#include "../../lib/core/error.h"
#include "../../lib/net/file_sink.h"
#include "../../lib/testing/host/fake_file_system.h"
#include "../../lib/testing/host/minitest.h"
#include "../../repository/fs/fs_page_source.h"
#include "../../repository/fs/fs_page_store.h"

static const char Page[] = "TITLE=K\xC3\xA9zik\xC3\xB6nyv\r\nROUTE=/x\r\nURL=https://wiki.example/x\r\n"
                           "L=1 #7\r\nL=2 https://example.com/a b\r\n\r\n"
                           "first line\r\nsee \x02"
                           "1\x03the page\x04\r\n\r\n\x01"
                           "bbold \xC5\x91\r\n";

TEST( pageStoreKeepsOnlyCompleteFiles ) {
  FakeFileSystem fs;
  Error error;
  FsPageStore store( fs, error );
  store.setRoot( "C:\\TTGWIKI\\CACHE" );
  CHECK( store.prepare( "EN" ) );
  FileSink *sink = store.beginFile( "EN" );
  CHECK( sink != NULL );
  sink->write( "abc", 3, error );
  CHECK( !store.endFile( "EN", "P0000001.TXT", false ) );
  CHECK( !store.has( "EN", "P0000001.TXT" ) );
  sink = store.beginFile( "EN" );
  sink->write( "abc", 3, error );
  CHECK( store.endFile( "EN", "P0000001.TXT", true ) );
  CHECK_STR( fs.data( "C:\\TTGWIKI\\CACHE\\EN\\P0000001.TXT" ), "abc" );
  CHECK( !fs.has( "C:\\TTGWIKI\\CACHE\\EN\\_NEW.TMP" ) );
  store.remove( "EN", "P0000001.TXT" );
  CHECK( !store.has( "EN", "P0000001.TXT" ) );
}

TEST( pageSourceReadsLinesHeaderAndLinks ) {
  FakeFileSystem fs;
  Error error;
  FsPageStore store( fs, error );
  store.setRoot( "C:\\CACHE" );
  fs.add( "C:\\CACHE\\HU\\P0000003.TXT", Page );
  static long offsets[100];
  static unsigned char bits[13];
  FsPageSource source( fs, store, error, offsets, bits, 100 );
  source.setCodePage( CodePage852 );
  CHECK( source.open( "HU", 3 ) );
  CHECK_INT( source.id(), 3 );
  CHECK_STR( source.title(), "K\x82zik\x94nyv" );
  CHECK_STR( source.url(), "https://wiki.example/x" );
  CHECK_INT( source.lineCount(), 4 );
  char line[100];
  CHECK( source.readLine( 0, line, sizeof( line ) ) );
  CHECK_STR( line, "first line" );
  CHECK( source.readLine( 2, line, sizeof( line ) ) );
  CHECK_STR( line, "" );
  CHECK( source.readLine( 3, line, sizeof( line ) ) );
  CHECK_STR( line, "\x01"
                   "bbold \x8B" );
  CHECK( !source.hasLinks( 0 ) );
  CHECK( source.hasLinks( 1 ) );
  CHECK( source.linkTarget( 2, line, sizeof( line ) ) );
  CHECK_STR( line, "https://example.com/a b" );
  CHECK( source.linkTarget( 1, line, sizeof( line ) ) );
  CHECK_STR( line, "#7" );
  CHECK( !source.linkTarget( 3, line, sizeof( line ) ) );
  CHECK( !source.truncated() );
  source.close();
  CHECK_INT( fs.openCount(), 0 );
}

TEST( pageSourceLeavesOutWhatDoesNotFit ) {
  FakeFileSystem fs;
  Error error;
  FsPageStore store( fs, error );
  store.setRoot( "C:\\CACHE" );
  fs.add( "C:\\CACHE\\EN\\P0000001.TXT", Page );
  static long offsets[8];
  static unsigned char bits[1];
  FsPageSource source( fs, store, error, offsets, bits, 8 );
  CHECK( source.open( "EN", 1 ) );
  CHECK( source.truncated() );
  CHECK_INT( source.lineCount(), 2 );
  CHECK( !source.open( "EN", 2 ) );
  CHECK_STR( error.text(), "Cannot read C:\\CACHE\\EN\\P0000002.TXT" );
}
