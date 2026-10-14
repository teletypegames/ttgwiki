#include <string.h>

#include "fs_page_store.h"
#include "../../lib/core/error.h"
#include "../../lib/core/str.h"
#include "../../lib/platform/file_system.h"

static const char TempName[] = "_NEW.TMP";

FsPageStore::FsPageStore( FileSystem &fs, Error &error ) : fs_( fs ), error_( error ), file_( fs ) {
  root_[0] = 0;
}

void FsPageStore::setRoot( const char *root ) {
  copyStr( root_, root, sizeof( root_ ) );
}

void FsPageStore::path( const char *lang, const char *name, char *out ) {
  joinLocal( out, root_, lang );
  if ( *name ) joinLocal( out, out, name );
}

bool FsPageStore::prepare( const char *lang ) {
  char folder[128];
  path( lang, "", folder );
  if ( makeFolders( fs_, folder ) ) return true;
  error_.set( "Cannot create %s", folder );
  return false;
}

int FsPageStore::readText( const char *lang, const char *name, char *buf, unsigned size ) {
  char file[128];
  path( lang, name, file );
  return ::readText( fs_, file, buf, size );
}

bool FsPageStore::writeText( const char *lang, const char *name, const char *text ) {
  char file[128];
  path( lang, name, file );
  if ( writeFile( fs_, file, text, strlen( text ) ) ) return true;
  error_.set( "Cannot write %s", file );
  return false;
}

bool FsPageStore::has( const char *lang, const char *name ) {
  char file[128];
  path( lang, name, file );
  return fs_.exists( file );
}

void FsPageStore::remove( const char *lang, const char *name ) {
  char file[128];
  path( lang, name, file );
  fs_.remove( file );
}

FileSink *FsPageStore::beginFile( const char *lang ) {
  char file[128];
  path( lang, TempName, file );
  if ( file_.open( file ) ) return &file_;
  error_.set( "Cannot write %s", file );
  return NULL;
}

bool FsPageStore::endFile( const char *lang, const char *name, bool keep ) {
  char temp[128], file[128];
  path( lang, TempName, temp );
  path( lang, name, file );
  if ( !file_.close() && keep ) {
    error_.set( "Cannot write %s (disk full?)", file );
    keep = false;
  }
  if ( !keep ) {
    fs_.remove( temp );
    return false;
  }
  fs_.remove( file );
  if ( fs_.rename( temp, file ) ) return true;
  error_.set( "Cannot write %s", file );
  fs_.remove( temp );
  return false;
}
