#include <stdio.h>
#include <string.h>

#include "sync_service.h"
#include "../lib/core/error.h"
#include "../lib/core/progress.h"
#include "../lib/core/str.h"
#include "../lib/net/file_sink.h"
#include "../model/config.h"
#include "../model/manifest.h"
#include "../model/page_index.h"
#include "../repository/page_store.h"
#include "../repository/remote_wiki.h"

SyncService::SyncService( RemoteWiki &remote, PageStore &store, PageIndex &index, const SyncBuffers &buffers,
                          const Config &config, Progress &progress, Error &error )
  : remote_( remote ), store_( store ), index_( index ), buffers_( buffers ), config_( config ), progress_( progress ),
    error_( error ) {
  buffers_.manifest[0] = buffers_.incoming[0] = buffers_.info[0] = 0;
}

const WikiInfo &SyncService::info() const {
  return info_;
}

static void remoteFile( const char *lang, const char *name, char *out ) {
  joinRemote( out, lang, name );
}

bool SyncService::loadIndex( const char *lang ) {
  index_.clear();
  if ( store_.readText( lang, IndexFile, index_.text(), PageIndex::TextSize ) < 0 ) return false;
  index_.parse( config_.codePage );
  return index_.count() > 0;
}

bool SyncService::loadLocal( const char *lang ) {
  buffers_.manifest[0] = 0;
  store_.readText( lang, ManifestFile, buffers_.manifest, Manifest::Size );
  if ( store_.readText( lang, WikiInfoFile, buffers_.info, InfoSize ) >= 0 ) info_.read( buffers_.info );
  return loadIndex( lang );
}

/* Deletes the pages on the disk that the new FILES.LST (incoming) does not describe as
   they are: changed, gone, or there without a FILES.LST to vouch for them. */
void SyncService::dropStale( const char *lang ) {
  Manifest old( buffers_.manifest[0] ? buffers_.manifest : NULL );
  Manifest fresh( buffers_.incoming );
  ManifestEntry entry;
  const char *pos = old.first();
  while ( old.next( pos, entry ) ) {
    if ( !old.sameIn( fresh, entry ) ) store_.remove( lang, entry.name );
  }
  pos = fresh.first();
  while ( fresh.next( pos, entry ) ) {
    if ( !fresh.sameIn( old, entry ) && store_.has( lang, entry.name ) ) store_.remove( lang, entry.name );
  }
}

bool SyncService::download( const char *lang, const char *name, const char *manifestText ) {
  ManifestEntry entry;
  if ( !Manifest( manifestText ).find( name, entry ) ) {
    error_.set( "%s\\%s is not in the export", lang, name );
    return false;
  }
  FileSink *sink = store_.beginFile( lang );
  if ( sink == NULL ) return false;
  char path[40], label[40];
  remoteFile( lang, name, path );
  snprintf( label, sizeof( label ), "%s %s", lang, name );
  bool ok = remote_.download( path, *sink, label );
  bool match = ok && sink->crc() == entry.crc && sink->size() == entry.size;
  if ( ok && !match ) error_.set( "%s: bad checksum", path );
  return store_.endFile( lang, name, match ) && match;
}

SyncService::Result SyncService::refreshConnected( const char *lang ) {
  reportf( progress_, "Reading %s", WikiInfoFile );
  char *incomingInfo = buffers_.incoming;
  if ( remote_.readText( WikiInfoFile, incomingInfo, InfoSize ) != RemoteWiki::Fetched ) return Failed;
  WikiInfo fresh;
  if ( !fresh.read( incomingInfo ) ) {
    error_.set( "%s is not a TTGWIKI export", WikiInfoFile );
    return Failed;
  }
  if ( !fresh.hasLanguage( lang ) ) {
    error_.set( "The wiki has no %s pages", lang );
    return Failed;
  }
  char infoText[InfoSize];
  copyStr( infoText, incomingInfo, sizeof( infoText ) );
  bool same = fresh.generation == info_.generation && store_.has( lang, IndexFile ) &&
              store_.has( lang, WikiInfoFile ) && buffers_.manifest[0];
  info_ = fresh;
  if ( same ) return Unchanged;
  if ( !store_.prepare( lang ) ) return Failed;
  char path[40];
  remoteFile( lang, ManifestFile, path );
  reportf( progress_, "Reading the %s index", lang );
  if ( remote_.readText( path, buffers_.incoming, Manifest::Size ) != RemoteWiki::Fetched ) return Failed;
  dropStale( lang );
  if ( !download( lang, IndexFile, buffers_.incoming ) ) return Failed;
  if ( !store_.writeText( lang, ManifestFile, buffers_.incoming ) ) return Failed;
  strcpy( buffers_.manifest, buffers_.incoming );
  if ( !store_.writeText( lang, WikiInfoFile, infoText ) ) return Failed;
  strcpy( buffers_.info, infoText );
  return Updated;
}

SyncService::Result SyncService::refresh( const char *lang ) {
  if ( store_.readText( lang, WikiInfoFile, buffers_.info, InfoSize ) >= 0 ) info_.read( buffers_.info );
  else info_.generation = 0;
  buffers_.manifest[0] = 0;
  store_.readText( lang, ManifestFile, buffers_.manifest, Manifest::Size );
  if ( !remote_.connect() ) return Failed;
  Result result = refreshConnected( lang );
  remote_.disconnect();
  if ( result != Failed && !loadIndex( lang ) ) {
    error_.set( "Cannot read the %s index", lang );
    return Failed;
  }
  return result;
}

bool SyncService::downloadPage( const char *lang, unsigned id ) {
  char name[13];
  pageFileName( id, name );
  if ( !remote_.connect() ) return false;
  reportf( progress_, "Downloading %s %s", lang, name );
  bool ok = download( lang, name, buffers_.manifest );
  remote_.disconnect();
  return ok;
}

bool SyncService::ensurePage( const char *lang, unsigned id ) {
  char name[13];
  pageFileName( id, name );
  if ( store_.has( lang, name ) ) return true;
  if ( downloadPage( lang, id ) ) return true;
  /* The export may have changed since the last refresh: once more with the new one. */
  char first[Error::Size];
  copyStr( first, error_.text(), sizeof( first ) );
  if ( refresh( lang ) != Updated ) {
    error_.set( "%s", first );
    return false;
  }
  return downloadPage( lang, id );
}

int SyncService::downloadAll( const char *lang ) {
  Manifest manifest( buffers_.manifest );
  ManifestEntry entry;
  int missing = 0, done = 0;
  const char *pos = manifest.first();
  while ( manifest.next( pos, entry ) ) {
    if ( !store_.has( lang, entry.name ) ) missing++;
  }
  if ( missing == 0 ) return 0;
  if ( !remote_.connect() ) return -1;
  pos = manifest.first();
  while ( manifest.next( pos, entry ) ) {
    if ( store_.has( lang, entry.name ) ) continue;
    reportf( progress_, "Downloading %d of %d", done + 1, missing );
    if ( !download( lang, entry.name, buffers_.manifest ) ) {
      remote_.disconnect();
      return -1;
    }
    done++;
  }
  remote_.disconnect();
  return done;
}

int SyncService::pagesOnDisk( const char *lang ) {
  int n = 0;
  for ( int i = 0; i < index_.count(); i++ ) {
    char name[13];
    pageFileName( index_.at( i ).id, name );
    if ( store_.has( lang, name ) ) n++;
  }
  return n;
}
