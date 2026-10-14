/*
   INDEX.TXT of a language: the page tree, a page per line in tree order (a parent before
   its children), with tab-separated fields:

     id  parent-id  depth  modified  locale  route  title

   The text is UTF-8; the titles are turned into the code page of the screen.
*/

#ifndef TTGWIKI_MODEL_PAGE_INDEX_H
#define TTGWIKI_MODEL_PAGE_INDEX_H

#include "../lib/core/code_page.h"

static const char IndexFile[] = "INDEX.TXT";

struct PageEntry {
  unsigned id;
  unsigned parent; /* the parent's id, 0 for none */
  unsigned char depth;
  unsigned long modified; /* Unix time */
  const char *locale;     /* the language the page is written in */
  const char *route;      /* "/dos/gmenu" */
  const char *title;
};

class PageIndex {
public:
  enum { MaxPages = 600, TextSize = 40000u };

  /* text: a buffer of TextSize bytes that the index keeps and fills */
  explicit PageIndex( char *text );
  /* Takes the INDEX.TXT in the buffer (terminated): the pages in it. */
  int parse( CodePage cp );
  void clear();
  char *text();
  int count() const;
  const PageEntry &at( int i ) const;
  /* The index of a page, -1 when it is not in the tree. */
  int find( unsigned id ) const;
  int findRoute( const char *route ) const;

private:
  bool parseLine( char *line, PageEntry &entry, CodePage cp );

  char *text_;
  PageEntry pages_[MaxPages];
  int count_;
};

#endif
