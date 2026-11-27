#include <string.h>

#include "search_service.h"
#include "sync_service.h"
#include "../lib/core/progress.h"
#include "../model/word_query.h"
#include "../repository/line_file.h"

SearchService::SearchService( SyncService &sync, LineFile &file, Progress &progress )
  : sync_( sync ), file_( file ), progress_( progress ), count_( 0 ) {
  clear();
}

void SearchService::clear() {
  memset( found_, 0, sizeof( found_ ) );
  count_ = 0;
}

bool SearchService::found( unsigned id ) const {
  return id < MaxId && ( found_[id >> 3] & ( 1 << ( id & 7 ) ) ) != 0;
}

int SearchService::foundCount() const {
  return count_;
}

bool SearchService::collect( const char *lang, const char *word ) {
  memset( word_, 0, sizeof( word_ ) );
  char name[13];
  wordFileName( word, name );
  if ( !sync_.inExport( name ) ) return true;
  if ( !sync_.ensureFile( lang, name ) ) return false;
  if ( !file_.open( lang, name ) ) return false;
  reportf( progress_, "Searching %s", word );
  char line[WordQuery::LineSize];
  unsigned length = strlen( word );
  while ( file_.next( line, sizeof( line ) ) ) {
    if ( strncmp( line, word, length ) > 0 ) break;
    const char *ids = wordLineIds( line, word );
    unsigned id;
    while ( ids && nextWordId( ids, id ) ) {
      if ( id < MaxId ) word_[id >> 3] |= (unsigned char)( 1 << ( id & 7 ) );
    }
  }
  file_.close();
  return true;
}

SearchService::Result SearchService::search( const char *lang, const WordQuery &query ) {
  clear();
  if ( query.count() == 0 ) return NoWords;
  if ( !sync_.hasWordIndex() ) return NoIndex;
  for ( int w = 0; w < query.count(); w++ ) {
    if ( !collect( lang, query.word( w ) ) ) {
      clear();
      return Failed;
    }
    for ( unsigned i = 0; i < sizeof( found_ ); i++ ) found_[i] = w == 0 ? word_[i] : found_[i] & word_[i];
  }
  for ( unsigned id = 0; id < MaxId; id++ ) {
    if ( found( id ) ) count_++;
  }
  return Searched;
}
