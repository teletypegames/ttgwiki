#include "reader_service.h"
#include "sync_service.h"
#include "../model/config.h"
#include "../repository/page_source.h"

ReaderService::ReaderService( SyncService &sync, PageSource &source, const Config &config )
  : sync_( sync ), source_( source ), config_( config ) {}

bool ReaderService::open( unsigned id ) {
  if ( !sync_.ensurePage( config_.lang, id ) ) return false;
  return source_.open( config_.lang, id );
}

void ReaderService::close() {
  source_.close();
}

bool ReaderService::isOpen() const {
  return source_.id() != 0;
}

unsigned ReaderService::id() const {
  return source_.id();
}

int ReaderService::lineCount() const {
  return source_.lineCount();
}

bool ReaderService::truncated() const {
  return source_.truncated();
}

bool ReaderService::readLine( int line, char *buf, unsigned size ) {
  return source_.readLine( line, buf, size );
}

bool ReaderService::hasLinks( int line ) const {
  return source_.hasLinks( line );
}

const char *ReaderService::title() const {
  return source_.title();
}

const char *ReaderService::url() const {
  return source_.url();
}

bool ReaderService::linkTarget( int link, char *out, unsigned size ) {
  return source_.linkTarget( link, out, size );
}
