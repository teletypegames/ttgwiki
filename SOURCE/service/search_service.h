/*
   The text search of a language (model/word_query.h): brings the word files of the
   query to the disk and finds the pages that have every word. The pages found are kept
   by their ids (up to MaxId - 1) until the next search.
*/

#ifndef TTGWIKI_SERVICE_SEARCH_SERVICE_H
#define TTGWIKI_SERVICE_SEARCH_SERVICE_H

class LineFile;
class Progress;
class SyncService;
class WordQuery;

class SearchService {
public:
  enum { MaxId = 8192 };
  enum Result { Searched, NoWords, NoIndex, Failed };

  SearchService( SyncService &sync, LineFile &file, Progress &progress );
  /* NoWords: the query has no word of 3 letters; NoIndex: the export has no word files
     (an older exporter); Failed: a word file could not be had (the Error says why). */
  Result search( const char *lang, const WordQuery &query );
  void clear();
  bool found( unsigned id ) const;
  int foundCount() const;

private:
  bool collect( const char *lang, const char *word );

  SyncService &sync_;
  LineFile &file_;
  Progress &progress_;
  unsigned char found_[MaxId / 8];
  unsigned char word_[MaxId / 8];
  int count_;
};

#endif
