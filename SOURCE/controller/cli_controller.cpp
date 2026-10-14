#include <stdio.h>

#include "cli_controller.h"
#include "../lib/core/error.h"
#include "../lib/core/str.h"
#include "../model/config.h"
#include "../model/page_index.h"
#include "../service/reader_service.h"
#include "../service/sync_service.h"
#include "../view/cli_view.h"

CliController::CliController( SyncService &sync, ReaderService &reader, PageIndex &index, CliView &view, Config &config,
                              Error &error )
  : sync_( sync ), reader_( reader ), index_( index ), view_( view ), config_( config ), error_( error ) {}

void CliController::useLanguage( const char *lang ) {
  if ( lang == NULL ) return;
  copyStr( config_.lang, lang, sizeof( config_.lang ) );
  upcaseStr( config_.lang );
}

int CliController::fail() {
  view_.printError( error_ );
  error_.clear();
  return 1;
}

/* The index from the server, or the one on the disk when the server cannot be reached. */
bool CliController::loadIndex() {
  sync_.loadLocal( config_.lang );
  if ( sync_.refresh( config_.lang ) != SyncService::Failed ) return true;
  if ( index_.count() == 0 ) return false;
  char msg[120];
  snprintf( msg, sizeof( msg ), "Offline (%s): the pages on the disk", error_.text() );
  view_.printMessage( msg );
  error_.clear();
  return true;
}

int CliController::sync( const char *lang ) {
  useLanguage( lang );
  if ( sync_.refresh( config_.lang ) == SyncService::Failed ) return fail();
  int done = sync_.downloadAll( config_.lang );
  if ( done < 0 ) return fail();
  char msg[80];
  snprintf( msg, sizeof( msg ), "%s: export #%lu, %d pages, %d downloaded", config_.lang, sync_.info().generation,
            index_.count(), done );
  view_.printMessage( msg );
  return 0;
}

int CliController::show( const char *route, const char *lang ) {
  useLanguage( lang );
  if ( !loadIndex() ) return fail();
  int i = index_.findRoute( route );
  if ( i < 0 ) {
    error_.set( "No page %s", route );
    return fail();
  }
  if ( !reader_.open( index_.at( i ).id ) ) return fail();
  char line[400];
  for ( int n = 0; n < reader_.lineCount(); n++ ) {
    if ( !reader_.readLine( n, line, sizeof( line ) ) ) break;
    view_.printPageLine( line );
  }
  reader_.close();
  return 0;
}
