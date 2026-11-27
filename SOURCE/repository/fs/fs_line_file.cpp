#include "fs_line_file.h"
#include "../page_store.h"
#include "../../lib/core/error.h"
#include "../../lib/platform/file_system.h"

FsLineFile::FsLineFile( FileSystem &fs, PageStore &store, Error &error )
  : fs_( fs ), store_( store ), error_( error ), handle_( -1 ), length_( 0 ), pos_( 0 ) {}

bool FsLineFile::open( const char *lang, const char *name ) {
  close();
  char path[128];
  store_.path( lang, name, path );
  handle_ = fs_.openRead( path );
  if ( handle_ < 0 ) {
    error_.set( "Cannot read %s", path );
    return false;
  }
  length_ = pos_ = 0;
  return true;
}

bool FsLineFile::fill() {
  if ( pos_ < length_ ) return true;
  if ( handle_ < 0 ) return false;
  length_ = fs_.read( handle_, buffer_, sizeof( buffer_ ) );
  pos_ = 0;
  if ( length_ > 0 ) return true;
  length_ = 0;
  return false;
}

bool FsLineFile::next( char *buf, unsigned size ) {
  unsigned n = 0;
  bool any = false;
  while ( fill() ) {
    any = true;
    char c = buffer_[pos_++];
    if ( c == '\n' ) break;
    if ( c != '\r' && n + 1 < size ) buf[n++] = c;
  }
  buf[n] = 0;
  return any;
}

void FsLineFile::close() {
  if ( handle_ >= 0 ) fs_.close( handle_ );
  handle_ = -1;
  length_ = pos_ = 0;
}
