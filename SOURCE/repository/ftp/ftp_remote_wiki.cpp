#include <string.h>

#include "ftp_remote_wiki.h"
#include "../../lib/core/error.h"
#include "../../lib/core/progress.h"
#include "../../lib/core/str.h"
#include "../../lib/net/byte_sink.h"
#include "../../lib/platform/network.h"
#include "../../model/config.h"

FtpRemoteWiki::FtpRemoteWiki( Network &net, Clock &clock, const Config &config, Progress &progress, Error &error )
  : net_( net ), config_( config ), progress_( progress ), error_( error ), ftp_( net, clock, error ), netUp_( false ),
    connected_( false ) {}

bool FtpRemoteWiki::startNetwork() {
  if ( netUp_ ) return true;
  const char *err = net_.start();
  if ( err ) {
    error_.set( "%s", err );
    return false;
  }
  netUp_ = true;
  reportf( progress_, "Resolving %s", config_.server );
  if ( !net_.resolve( config_.server, ip_ ) ) {
    shutdown();
    error_.set( "Cannot resolve %s", config_.server );
    return false;
  }
  return true;
}

bool FtpRemoteWiki::connect() {
  if ( !startNetwork() ) return false;
  reportf( progress_, "Connecting to %s", config_.server );
  connected_ = ftp_.open( config_.server, ip_, config_.port, config_.user, config_.password );
  return connected_;
}

void FtpRemoteWiki::disconnect() {
  if ( connected_ ) ftp_.close();
  connected_ = false;
}

void FtpRemoteWiki::shutdown() {
  disconnect();
  if ( netUp_ ) net_.stop();
  netUp_ = false;
}

void FtpRemoteWiki::remotePath( const char *path, char *out ) const {
  joinRemote( out, config_.root, path );
}

RemoteWiki::Fetch FtpRemoteWiki::readText( const char *path, char *buf, unsigned size ) {
  char full[160];
  remotePath( path, full );
  MemorySink sink( buf, size );
  int code = ftp_.retrieve( full, sink, NULL, NULL );
  if ( code > 0 ) {
    error_.set( "%s is not on the server", full );
    return Missing;
  }
  return code == 0 ? Fetched : Failed;
}

bool FtpRemoteWiki::download( const char *path, ByteSink &sink, const char *label ) {
  char full[160];
  remotePath( path, full );
  int code = ftp_.retrieve( full, sink, label, this );
  if ( code > 0 ) error_.set( "Cannot download %s: %s", full, ftp_.reply() );
  return code == 0;
}

void FtpRemoteWiki::transferred( const char *label, unsigned long bytes ) {
  reportf( progress_, "%s  %lu bytes", label, bytes );
}
