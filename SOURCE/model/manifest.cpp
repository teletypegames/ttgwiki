#include <stdio.h>
#include <string.h>

#include "manifest.h"
#include "../lib/core/str.h"

Manifest::Manifest( const char *text ) : text_( text ) {}

bool Manifest::isPresent() const {
  return text_ != NULL;
}

const char *Manifest::first() const {
  return text_ ? text_ : "";
}

bool Manifest::next( const char *&pos, ManifestEntry &entry ) const {
  while ( *pos ) {
    const char *line = pos;
    const char *end = strchr( line, '\n' );
    pos = end ? end + 1 : line + strlen( line );
    unsigned long crc, size;
    if ( sscanf( line, "%lx %lu %12s", &crc, &size, entry.name ) == 3 ) {
      entry.crc = crc;
      entry.size = size;
      return true;
    }
  }
  return false;
}

bool Manifest::find( const char *name, ManifestEntry &entry ) const {
  const char *pos = first();
  while ( next( pos, entry ) ) {
    if ( cmpi( entry.name, name ) == 0 ) return true;
  }
  return false;
}

bool Manifest::sameIn( const Manifest &other, const ManifestEntry &entry ) const {
  ManifestEntry theirs;
  return other.find( entry.name, theirs ) && theirs.crc == entry.crc && theirs.size == entry.size;
}
