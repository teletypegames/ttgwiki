/*
   FILES.LST of a language folder of the export: "crc32 size name" per line (hex CRC-32,
   decimal size), for INDEX.TXT and every page.
*/

#ifndef TTGWIKI_MODEL_MANIFEST_H
#define TTGWIKI_MODEL_MANIFEST_H

#include <stdint.h>

static const char ManifestFile[] = "FILES.LST";

struct ManifestEntry {
  char name[13];
  uint32_t crc;
  uint32_t size;
};

class Manifest {
public:
  enum { Size = 16384u };

  /* text: the file, or NULL when there is none */
  explicit Manifest( const char *text );
  bool isPresent() const;
  bool find( const char *name, ManifestEntry &entry ) const;
  /* The entries one after the other: for ( p = m.first(); m.next( p, e ); ) */
  const char *first() const;
  bool next( const char *&pos, ManifestEntry &entry ) const;
  /* The entry is the same in other (a file that did not change). */
  bool sameIn( const Manifest &other, const ManifestEntry &entry ) const;

private:
  const char *text_;
};

#endif
