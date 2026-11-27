/*
   The text search. The export has the words of a language's pages in a file for each
   first character, WORDS_A.TXT ... WORDS_Z.TXT and WORDS_0.TXT ... WORDS_9.TXT (only
   the ones that have words, all of them in FILES.LST). A word is folded (upper case
   without accents, A-Z and 0-9 only) and has 3 to 30 characters. A line is a word and
   the ids of the pages that have it, ascending:

     GMENU 3,12,40

   A long id list goes on in another line of the same word; a line is at most 250 bytes.

   A query is the words typed, folded the same way; a page is found when it has every
   word of the query, as a word or as the start of one (DOS finds DOSBOX too).
*/

#ifndef TTGWIKI_MODEL_WORD_QUERY_H
#define TTGWIKI_MODEL_WORD_QUERY_H

#include "../lib/core/code_page.h"

class WordQuery {
public:
  enum { MaxWords = 8, MinLength = 3, MaxLength = 30, LineSize = 256 };

  WordQuery();
  /* The words of text (in the code page cp); shorter ones than MinLength are left out,
     longer ones than MaxLength cut. How many there are. */
  int parse( const char *text, CodePage cp );
  int count() const;
  const char *word( int i ) const;

private:
  char words_[MaxWords][MaxLength + 1];
  int count_;
};

/* The word file of a word: WORDS_A.TXT */
void wordFileName( const char *word, char out[13] );
/* The name is a word file. */
bool isWordFile( const char *name );
/* The ids of a word file line when its word starts with word (folded), NULL when not. */
const char *wordLineIds( const char *line, const char *word );
/* The next id of a list ("3,12,40"): false at its end. */
bool nextWordId( const char *&pos, unsigned &id );

#endif
