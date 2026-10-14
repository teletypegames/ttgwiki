#include <string.h>

#include "fakes.h"
#include "../../lib/core/error.h"
#include "../../lib/testing/host/fake_file_system.h"
#include "../../lib/testing/host/minitest.h"
#include "../../model/config.h"
#include "../../model/manifest.h"
#include "../../model/page_index.h"
#include "../../repository/fs/fs_page_source.h"
#include "../../repository/fs/fs_page_store.h"
#include "../../service/reader_service.h"
#include "../../service/sync_service.h"

static const char Index[] = "1\t0\t0\t100\ten\t/\tHome\r\n2\t1\t1\t100\ten\t/dos\tDOS\r\n";

/* A wiki of two pages on a fake server, and TTGWIKI's services on a fake disk. */
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

struct World : TestBuffers {
  World()
    : index( indexText ), store( fs, error ), source( fs, store, error, offsets, bits, 200 ),
      sync( remote, store, index, buffers(), config, progress, error ), reader( sync, source, config ) {
    store.setRoot( "C:\\TTGWIKI\\CACHE" );
    remote.putInfo( 1 );
    remote.putIndex( "EN", Index );
    remote.putPage( "EN", 1, "Home", "L=1 #2\r\n",
                    "Welcome, see \x02"
                    "1\x03"
                    "DOS\x04\r\n" );
    remote.putPage( "EN", 2, "DOS", "", "DOS programs\r\n" );
    remote.putManifest( "EN" );
  }

  SyncBuffers buffers() {
    SyncBuffers b = { manifest, incoming, info };
    return b;
  }

  bool cached( const char *name ) {
    char path[80];
    strcpy( path, "C:\\TTGWIKI\\CACHE\\EN\\" );
    strcat( path, name );
    return fs.has( path );
  }

  FakeRemote remote;
  FakeFileSystem fs;
  Error error;
  Config config;
  SilentProgress progress;
  PageIndex index;
  FsPageStore store;
  FsPageSource source;
  SyncService sync;
  ReaderService reader;
};

TEST( firstRefreshBringsTheIndex ) {
  static World w;
  CHECK( !w.sync.loadLocal( "EN" ) );
  CHECK( w.sync.refresh( "EN" ) == SyncService::Updated );
  CHECK_INT( w.index.count(), 2 );
  CHECK_STR( w.index.at( 1 ).title, "DOS" );
  CHECK( w.cached( "INDEX.TXT" ) && w.cached( "FILES.LST" ) && w.cached( "WIKI.INI" ) );
  CHECK( !w.cached( "P0000001.TXT" ) );
  CHECK_INT( (long)w.sync.info().generation, 1 );
  CHECK( !w.remote.connected );
}

TEST( sameGenerationReadsOnlyWikiIni ) {
  static World w;
  w.sync.refresh( "EN" );
  int downloads = w.remote.downloads;
  CHECK( w.sync.refresh( "EN" ) == SyncService::Unchanged );
  CHECK_INT( w.remote.downloads, downloads );
  CHECK_INT( w.index.count(), 2 );
}

TEST( pagesAreDownloadedWhenReadAndKept ) {
  static World w;
  w.sync.refresh( "EN" );
  CHECK( w.reader.open( 1 ) );
  CHECK( w.cached( "P0000001.TXT" ) );
  CHECK_STR( w.reader.title(), "Home" );
  CHECK_INT( w.reader.lineCount(), 1 );
  int downloads = w.remote.downloads;
  w.remote.online = false;
  CHECK( w.reader.open( 1 ) );
  CHECK_INT( w.remote.downloads, downloads );
}

TEST( aNewGenerationDropsTheChangedPages ) {
  static World w;
  w.sync.refresh( "EN" );
  w.reader.open( 1 );
  w.reader.open( 2 );
  w.reader.close();
  w.remote.putPage( "EN", 2, "DOS", "", "DOS programs, changed\r\n" );
  w.remote.putManifest( "EN" );
  w.remote.putInfo( 2 );
  CHECK( w.sync.refresh( "EN" ) == SyncService::Updated );
  CHECK( w.cached( "P0000001.TXT" ) );
  CHECK( !w.cached( "P0000002.TXT" ) );
  CHECK( w.reader.open( 2 ) );
  char line[80];
  w.reader.readLine( 0, line, sizeof( line ) );
  CHECK_STR( line, "DOS programs, changed" );
}

TEST( aPageThatDoesNotMatchIsNotKept ) {
  static World w;
  w.sync.refresh( "EN" );
  w.remote.putPage( "EN", 2, "DOS", "", "changed behind FILES.LST\r\n" );
  CHECK( !w.reader.open( 2 ) );
  CHECK( strstr( w.error.text(), "bad checksum" ) != NULL );
  CHECK( !w.cached( "P0000002.TXT" ) );
  CHECK( !w.cached( "_NEW.TMP" ) );
}

TEST( aPageOfANewerExportIsFoundAfterARefresh ) {
  static World w;
  w.sync.refresh( "EN" );
  w.remote.putPage( "EN", 2, "DOS", "", "newer\r\n" );
  w.remote.putManifest( "EN" );
  w.remote.putInfo( 2 );
  CHECK( w.reader.open( 2 ) );
  CHECK_INT( (long)w.sync.info().generation, 2 );
}

TEST( offlineTheDiskIsEnough ) {
  static World w;
  w.sync.refresh( "EN" );
  w.reader.open( 1 );
  w.remote.online = false;
  CHECK( w.sync.loadLocal( "EN" ) );
  CHECK( w.sync.refresh( "EN" ) == SyncService::Failed );
  CHECK_INT( w.index.count(), 2 );
  CHECK( w.reader.open( 1 ) );
  CHECK( !w.reader.open( 2 ) );
}

TEST( downloadAllBringsEveryPageOnce ) {
  static World w;
  w.sync.refresh( "EN" );
  CHECK_INT( w.sync.downloadAll( "EN" ), 2 );
  CHECK( w.cached( "P0000001.TXT" ) && w.cached( "P0000002.TXT" ) );
  CHECK_INT( w.sync.downloadAll( "EN" ), 0 );
  CHECK_INT( w.sync.pagesOnDisk( "EN" ), 2 );
}

TEST( aMissingLanguageIsAnError ) {
  static World w;
  CHECK( w.sync.refresh( "DE" ) == SyncService::Failed );
  CHECK_STR( w.error.text(), "The wiki has no DE pages" );
}
