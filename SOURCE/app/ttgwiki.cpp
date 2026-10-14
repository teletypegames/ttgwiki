#include <stddef.h>
#include <string.h>

#include "ttgwiki.h"
#include "version.h"
#include "../controller/browser_controller.h"
#include "../controller/cli_controller.h"
#include "../lib/core/error.h"
#include "../lib/core/str.h"
#include "../lib/platform/file_system.h"
#include "../lib/platform/idle.h"
#include "../lib/platform/network.h"
#include "../lib/ui/canvas.h"
#include "../model/config.h"
#include "../model/manifest.h"
#include "../model/page_index.h"
#include "../repository/fs/fs_page_source.h"
#include "../repository/fs/fs_page_store.h"
#include "../repository/ftp/ftp_remote_wiki.h"
#include "../service/reader_service.h"
#include "../service/sync_service.h"
#include "../view/cli_view.h"
#include "../view/wiki_view.h"

enum { MaxLines = 6000 };

class NetworkIdle : public Idle {
public:
  explicit NetworkIdle( Network &network ) : network_( network ) {}
  void idle() { network_.idle(); }

private:
  Network &network_;
};

/* The progress goes to the view of the command, which only exists after the config. */
class ProgressRelay : public Progress {
public:
  ProgressRelay() : target( NULL ) {}
  void report( const char *text ) {
    if ( target ) target->report( text );
  }
  Progress *target;
};

/* The big buffers, each a block of its own (none may reach 64 KB on DOS). */
struct Buffers {
  Buffers();
  ~Buffers();
  bool ok() const;

  char *indexText;
  char *manifest;
  char *incoming;
  char *info;
  long *offsets;
  unsigned char *linkBits;
};

Buffers::Buffers()
  : indexText( new char[PageIndex::TextSize] ), manifest( new char[Manifest::Size] ),
    incoming( new char[Manifest::Size] ), info( new char[SyncService::InfoSize] ), offsets( new long[MaxLines] ),
    linkBits( new unsigned char[MaxLines / 8 + 1] ) {}

Buffers::~Buffers() {
  delete[] indexText;
  delete[] manifest;
  delete[] incoming;
  delete[] info;
  delete[] offsets;
  delete[] linkBits;
}

bool Buffers::ok() const {
  return indexText && manifest && incoming && info && offsets && linkBits;
}

static SyncBuffers syncBuffers( Buffers &b ) {
  SyncBuffers s = { b.manifest, b.incoming, b.info };
  return s;
}

/* TTGWIKI put together: the members are built in this order. */
struct Ttgwiki {
  Ttgwiki( Machine &m, Buffers &b, Progress &progress );

  Error error;
  Config config;
  PageIndex index;
  FtpRemoteWiki remote;
  FsPageStore store;
  FsPageSource source;
  SyncService sync;
  ReaderService reader;
};

Ttgwiki::Ttgwiki( Machine &m, Buffers &b, Progress &progress )
  : index( b.indexText ), remote( m.network, m.clock, config, progress, error ), store( m.fs, error ),
    source( m.fs, store, error, b.offsets, b.linkBits, MaxLines ),
    sync( remote, store, index, syncBuffers( b ), config, progress, error ), reader( sync, source, config ) {}

static bool loadConfig( Machine &m, Ttgwiki &app ) {
  char path[160], text[2048];
  joinLocal( path, m.selfDir, ConfigFile );
  if ( readText( m.fs, path, text, sizeof( text ) ) >= 0 && !app.config.read( text ) ) {
    app.error.set( "CODEPAGE in %s: 437 or 852", path );
    return false;
  }
  char cache[128];
  if ( app.config.cache[0] ) copyStr( cache, app.config.cache, sizeof( cache ) );
  else joinLocal( cache, m.selfDir, "CACHE" );
  app.store.setRoot( cache );
  app.source.setCodePage( app.config.codePage );
  return true;
}

static void runBrowser( Machine &m, Ttgwiki &app, ProgressRelay &relay ) {
  Canvas canvas( m.screen );
  WikiView view( canvas, Version );
  NetworkIdle idle( m.network );
  BrowserController *browser =
      new BrowserController( app.sync, app.reader, app.index, view, m.keyboard, idle, app.config, app.error );
  if ( browser == NULL ) return;
  relay.target = &view;
  browser->run();
  relay.target = NULL;
  delete browser;
}

static bool isHelp( const char *arg ) {
  return strcmp( arg, "/?" ) == 0 || cmpi( arg, "-h" ) == 0;
}

static int runCommand( Machine &m, Ttgwiki &app, ProgressRelay &relay, int argc, char *argv[] ) {
  CliView cliView( m.out );
  CliController cli( app.sync, app.reader, app.index, cliView, app.config, app.error );
  if ( cmpi( argv[1], "SYNC" ) == 0 && argc <= 3 ) {
    relay.target = &cliView;
    return cli.sync( argc == 3 ? argv[2] : NULL );
  }
  if ( cmpi( argv[1], "SHOW" ) == 0 && ( argc == 3 || argc == 4 ) )
    return cli.show( argv[2], argc == 4 ? argv[3] : NULL );
  cliView.printUsage( Version );
  return 1;
}

int runTtgwiki( Machine &m, int argc, char *argv[] ) {
  CliView cliView( m.out );
  if ( argc > 1 && isHelp( argv[1] ) ) {
    cliView.printUsage( Version );
    return 0;
  }
  Buffers *buffers = new Buffers;
  ProgressRelay relay;
  Ttgwiki *app = buffers && buffers->ok() ? new Ttgwiki( m, *buffers, relay ) : NULL;
  if ( app == NULL ) {
    cliView.printMessage( "Not enough memory" );
    delete buffers;
    return 1;
  }
  int rc = 1;
  if ( !loadConfig( m, *app ) ) cliView.printError( app->error );
  else if ( argc == 1 ) {
    runBrowser( m, *app, relay );
    rc = 0;
  }
  else rc = runCommand( m, *app, relay, argc, argv );
  app->remote.shutdown();
  delete app;
  delete buffers;
  return rc;
}
