#include <string.h>

#include "fakes.h"
#include "../../controller/browser_controller.h"
#include "../../controller/cli_controller.h"
#include "../../lib/core/error.h"
#include "../../lib/platform/idle.h"
#include "../../lib/platform/text_output.h"
#include "../../lib/testing/host/fake_file_system.h"
#include "../../lib/testing/host/memory_screen.h"
#include "../../lib/testing/host/minitest.h"
#include "../../lib/ui/canvas.h"
#include "../../model/config.h"
#include "../../model/manifest.h"
#include "../../repository/fs/fs_page_source.h"
#include "../../repository/fs/fs_page_store.h"
#include "../../service/reader_service.h"
#include "../../service/sync_service.h"
#include "../../view/cli_view.h"

class NoIdle : public Idle {
public:
  void idle() {}
};

class CapturedOutput : public TextOutput {
public:
  CapturedOutput() { text[0] = 0; }
  void write( const char *s ) { strcat( text, s ); }
  char text[4000];
};

static const char Index[] = "1\t0\t0\t1791920030\ten\t/\tHome\r\n"
                            "2\t1\t1\t100\ten\t/dos\tDOS\r\n"
                            "3\t2\t2\t100\ten\t/dos/gmenu\tGMENU\r\n";

/* The browser on a fake server and disk: run( keys ) drives it until the keys run out. */
/* The big buffers, in a base built before the members that use them. */
struct TestBuffers {
  TestBuffers() { indexText[0] = manifest[0] = incoming[0] = info[0] = 0; }

  char indexText[PageIndex::TextSize];
  char manifest[Manifest::Size];
  char incoming[Manifest::Size];
  char info[SyncService::InfoSize];
  long offsets[200];
  unsigned char bits[26];
};

struct Browser : TestBuffers {
  Browser()
    : index( indexText ), store( fs, error ), source( fs, store, error, offsets, bits, 200 ),
      sync( remote, store, index, buffers(), config, progress, error ), reader( sync, source, config ),
      canvas( screen ), view( canvas, "0.0.0" ), kept( screen ) {
    store.setRoot( "C:\\CACHE" );
    remote.putInfo( 1 );
    remote.putIndex( "EN", Index );
    remote.putPage( "EN", 1, "Home", "L=1 #2\r\nL=2 https://example.com/\r\n",
                    "\x01"
                    "1Welcome\r\n\r\nSee \x02"
                    "1\x03the DOS tools\x04 and \x02"
                    "2\x03the web\x04.\r\n" );
    remote.putPage( "EN", 2, "DOS", "L=1 #3\r\n",
                    "DOS programs: \x02"
                    "1\x03GMENU\x04\r\n" );
    remote.putPage( "EN", 3, "GMENU", "", "A game menu\r\n\x07 one\r\n" );
    remote.putManifest( "EN" );
    remote.putIndex( "HU", "1\t0\t0\t100\thu\t/\tKezd\xC5\x91lap\r\n" );
    remote.putPage( "HU", 1, "Kezd\xC5\x91lap", "",
                    "\xC3\x9A"
                    "dv\xC3\xB6zl\xC3\xBCnk\r\n" );
    remote.putManifest( "HU" );
  }

  SyncBuffers buffers() {
    SyncBuffers b = { manifest, incoming, info };
    return b;
  }

  KeepScreen &run( const char *keys ) {
    ScriptedKeyboard keyboard( keys, kept );
    BrowserController *browser = new BrowserController( sync, reader, index, view, keyboard, idle, config, error );
    browser->run();
    delete browser;
    return kept;
  }

  FakeRemote remote;
  FakeFileSystem fs;
  Error error;
  Config config;
  SilentProgress progress;
  NoIdle idle;
  PageIndex index;
  FsPageStore store;
  FsPageSource source;
  SyncService sync;
  ReaderService reader;
  MemoryScreen screen;
  Canvas canvas;
  WikiView view;
  KeepScreen kept;
};

TEST( contentsShowTheTree ) {
  static Browser b;
  KeepScreen &s = b.run( "" );
  CHECK_STR( s.row( 0 ), " TTGWIKI 0.0.0 - Teletype Games Wiki                                    EN  #1" );
  CHECK_STR( s.row( 2 ), " Home                                                             2026-10-13" );
  CHECK( strncmp( s.row( 3 ), "   DOS ", 7 ) == 0 );
  CHECK( strncmp( s.row( 4 ), "     GMENU ", 11 ) == 0 );
  CHECK_STR( s.row( 23 ), " 3 pages, export #1 (new)" );
}

TEST( typingSearchesTheTitles ) {
  static Browser b;
  KeepScreen &s = b.run( "2267 226D" );
  CHECK( strncmp( s.row( 2 ), " GMENU ", 7 ) == 0 );
  CHECK_STR( s.row( 3 ), "" );
  CHECK( strncmp( s.row( 22 ), " Search: gm_", 12 ) == 0 );
}

TEST( enterReadsThePageAndTabSelectsALink ) {
  static Browser b;
  KeepScreen &s = b.run( "1C0D 0F09" );
  CHECK( strncmp( s.row( 0 ), " Home ", 6 ) == 0 );
  CHECK_STR( s.row( 1 ), " Welcome" );
  CHECK_STR( s.row( 3 ), " See the DOS tools and the web." );
  CHECK_STR( s.row( 23 ), " \x1A DOS" );
  CHECK_INT( s.attr( 3, 5 ), 0x70 );
  CHECK_INT( s.attr( 3, 23 ), 0x09 );
}

TEST( followingLinksAndGoingBack ) {
  static Browser b;
  KeepScreen &s = b.run( "1C0D 0F09 1C0D 0F09 1C0D" );
  CHECK( strncmp( s.row( 0 ), " GMENU ", 7 ) == 0 );
  CHECK_STR( s.row( 1 ), " A game menu" );
  static Browser c;
  KeepScreen &t = c.run( "1C0D 0F09 1C0D 0E08" );
  CHECK( strncmp( t.row( 0 ), " Home ", 6 ) == 0 );
  CHECK_STR( t.row( 23 ), " \x1A DOS" );
}

TEST( aWebLinkIsShownNotFollowed ) {
  static Browser b;
  KeepScreen &s = b.run( "1C0D 0F09 0F09 1C0D" );
  CHECK( strncmp( s.row( 0 ), " Home ", 6 ) == 0 );
  CHECK_STR( s.row( 23 ), " A web page, open it in a browser: https://example.com/" );
}

TEST( f2ShowsThePageInTheOtherLanguage ) {
  static Browser b;
  b.config.codePage = CodePage852;
  b.source.setCodePage( CodePage852 );
  KeepScreen &s = b.run( "1C0D 3C00" );
  CHECK( strncmp( s.row( 0 ), " Kezd\x8Blap ", 10 ) == 0 );
  CHECK_STR( s.row( 1 ), " \xE9"
                         "dv\x94zl\x81nk" );
  CHECK_STR( b.config.lang, "HU" );
}

TEST( offlineTheBrowserShowsTheError ) {
  static Browser b;
  b.remote.online = false;
  KeepScreen &s = b.run( "" );
  CHECK( strncmp( s.row( 23 ), " Error: ", 8 ) == 0 );
  CHECK_STR( s.row( 2 ), "" );
}

TEST( cliShowPrintsThePage ) {
  static Browser b;
  CapturedOutput out;
  CliView view( out );
  CliController cli( b.sync, b.reader, b.index, view, b.config, b.error );
  CHECK_INT( cli.show( "/", NULL ), 0 );
  CHECK_STR( out.text, "Welcome\n\nSee the DOS tools and the web.\n" );
  out.text[0] = 0;
  CHECK_INT( cli.show( "/dos/gmenu", NULL ), 0 );
  CHECK_STR( out.text, "A game menu\n* one\n" );
  out.text[0] = 0;
  CHECK_INT( cli.show( "/nothing", NULL ), 1 );
  CHECK_STR( out.text, "Error: No page /nothing\n" );
  out.text[0] = 0;
  CHECK_INT( cli.sync( "en" ), 0 );
  CHECK_STR( out.text, "EN: export #1, 3 pages, 1 downloaded\n" );
}
