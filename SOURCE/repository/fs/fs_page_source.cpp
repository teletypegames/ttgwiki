#include <stdlib.h>
#include <string.h>

#include "fs_page_source.h"
#include "../page_store.h"
#include "../../lib/core/error.h"
#include "../../lib/core/str.h"
#include "../../lib/platform/file_system.h"
#include "../../model/wiki_info.h"

FsPageSource::FsPageSource( FileSystem &fs, PageStore &store, Error &error, long *offsets, unsigned char *linkBits,
                            int maxLines )
  : fs_( fs ), store_( store ), error_( error ), cp_( CodePage437 ), offsets_( offsets ), linkBits_( linkBits ),
    maxLines_( maxLines ), handle_( -1 ), id_( 0 ), size_( 0 ), total_( 0 ), bodyStart_( 0 ), truncated_( false ),
    windowStart_( 0 ), windowLength_( 0 ) {
  title_[0] = url_[0] = 0;
}

void FsPageSource::setCodePage( CodePage cp ) {
  cp_ = cp;
}

void FsPageSource::close() {
  if ( handle_ >= 0 ) fs_.close( handle_ );
  handle_ = -1;
  id_ = 0;
  total_ = bodyStart_ = 0;
  windowLength_ = 0;
}

void FsPageSource::markLink( int line ) {
  linkBits_[line >> 3] |= (unsigned char)( 1 << ( line & 7 ) );
}

/* Where every line starts, and which lines have a link. */
bool FsPageSource::scan() {
  char buf[ScanSize];
  long pos = 0;
  total_ = 0;
  truncated_ = false;
  memset( linkBits_, 0, ( maxLines_ + 7 ) / 8 );
  bool lineStart = true;
  for ( ;; ) {
    int n = fs_.read( handle_, buf, sizeof( buf ) );
    if ( n < 0 ) return false;
    if ( n == 0 ) break;
    for ( int i = 0; i < n; i++ ) {
      if ( lineStart ) {
        if ( total_ == maxLines_ ) {
          truncated_ = true;
          size_ = pos + i;
          return true;
        }
        offsets_[total_++] = pos + i;
        lineStart = false;
      }
      if ( buf[i] == '\n' ) lineStart = true;
      else if ( buf[i] == 2 ) markLink( total_ - 1 );
    }
    pos += n;
  }
  size_ = pos;
  return true;
}

bool FsPageSource::loadWindow( long from ) {
  if ( !fs_.seek( handle_, from ) ) return false;
  int n = fs_.read( handle_, window_, sizeof( window_ ) );
  if ( n < 0 ) return false;
  windowStart_ = from;
  windowLength_ = (unsigned)n;
  return true;
}

bool FsPageSource::readRaw( int line, char *buf, unsigned size ) {
  buf[0] = 0;
  if ( line < 0 || line >= total_ ) return false;
  long from = offsets_[line];
  long to = line + 1 < total_ ? offsets_[line + 1] : size_;
  if ( (unsigned long)( to - from ) > size - 1 ) to = from + size - 1;
  if ( from < windowStart_ || to > windowStart_ + (long)windowLength_ ) {
    if ( !loadWindow( from ) ) return false;
    if ( to > windowStart_ + (long)windowLength_ ) to = windowStart_ + windowLength_;
  }
  unsigned len = (unsigned)( to - from );
  memcpy( buf, window_ + ( from - windowStart_ ), len );
  while ( len && ( buf[len - 1] == '\n' || buf[len - 1] == '\r' ) ) len--;
  buf[len] = 0;
  return true;
}

static const char *valueOf( const char *line, const char *key ) {
  int len = strlen( key );
  return strncmp( line, key, len ) == 0 ? line + len : NULL;
}

void FsPageSource::readHeader() {
  char line[256];
  title_[0] = url_[0] = 0;
  bodyStart_ = total_;
  for ( int i = 0; i < total_; i++ ) {
    readRaw( i, line, sizeof( line ) );
    if ( line[0] == 0 ) {
      bodyStart_ = i + 1;
      return;
    }
    const char *value;
    if ( ( value = valueOf( line, "TITLE=" ) ) != NULL ) {
      copyStr( title_, value, sizeof( title_ ) );
      utf8ToCodePage( title_, cp_ );
    }
    else if ( ( value = valueOf( line, "URL=" ) ) != NULL ) copyStr( url_, value, sizeof( url_ ) );
  }
}

bool FsPageSource::open( const char *lang, unsigned id ) {
  close();
  char name[13], file[128];
  pageFileName( id, name );
  store_.path( lang, name, file );
  handle_ = fs_.openRead( file );
  if ( handle_ < 0 ) {
    error_.set( "Cannot read %s", file );
    return false;
  }
  if ( !scan() ) {
    error_.set( "Cannot read %s", file );
    close();
    return false;
  }
  windowLength_ = 0;
  readHeader();
  id_ = id;
  return true;
}

unsigned FsPageSource::id() const {
  return id_;
}

int FsPageSource::lineCount() const {
  return total_ - bodyStart_;
}

bool FsPageSource::truncated() const {
  return truncated_;
}

bool FsPageSource::readLine( int line, char *buf, unsigned size ) {
  if ( !readRaw( bodyStart_ + line, buf, size ) ) return false;
  utf8ToCodePage( buf, cp_ );
  return true;
}

bool FsPageSource::hasLinks( int line ) const {
  int i = bodyStart_ + line;
  if ( i < 0 || i >= total_ ) return false;
  return ( linkBits_[i >> 3] & ( 1 << ( i & 7 ) ) ) != 0;
}

const char *FsPageSource::title() const {
  return title_;
}

const char *FsPageSource::url() const {
  return url_;
}

bool FsPageSource::linkTarget( int link, char *out, unsigned size ) {
  char line[256];
  for ( int i = 0; i < bodyStart_; i++ ) {
    readRaw( i, line, sizeof( line ) );
    const char *value = valueOf( line, "L=" );
    if ( value == NULL || atoi( value ) != link ) continue;
    const char *target = strchr( value, ' ' );
    if ( target == NULL ) return false;
    copyStr( out, target + 1, size );
    return true;
  }
  return false;
}
