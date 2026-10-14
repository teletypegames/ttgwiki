/*
   WIKI.INI at the root of the export: which export it is (the generation grows with
   every change) and its languages.
*/

#ifndef TTGWIKI_MODEL_WIKI_INFO_H
#define TTGWIKI_MODEL_WIKI_INFO_H

static const char WikiInfoFile[] = "WIKI.INI";

struct WikiInfo {
  enum { Format = 1, MaxLanguages = 8 };

  unsigned format;
  unsigned long generation;
  int languageCount;
  char languages[MaxLanguages][4];

  WikiInfo();
  /* false when it is not an export TTGWIKI can read */
  bool read( const char *ini );
  bool hasLanguage( const char *lang ) const;
  /* The language after lang (the first after the last one). */
  const char *nextLanguage( const char *lang ) const;
};

/* The file name of a page: P0000123.TXT */
void pageFileName( unsigned id, char out[13] );

#endif
