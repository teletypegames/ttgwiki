/*
   Keeps the pages on the disk in step with the export on the server. The rule that
   makes reading cheap: a page file in the store is always the one the store's
   FILES.LST describes. A refresh brings the new FILES.LST and deletes the pages that
   changed or went away, so a page that is there never has to be checked again; one that
   is not there is downloaded when it is read, and checked against FILES.LST.
*/

#ifndef TTGWIKI_SERVICE_SYNC_SERVICE_H
#define TTGWIKI_SERVICE_SYNC_SERVICE_H

#include "../model/wiki_info.h"

class Error;
class PageIndex;
class PageStore;
class Progress;
class RemoteWiki;
struct Config;
struct ManifestEntry;

/* The big buffers of the service, from the composition root. */
struct SyncBuffers {
  char *manifest; /* Manifest::Size: the store's FILES.LST */
  char *incoming; /* Manifest::Size: the server's, during a refresh */
  char *info;     /* InfoSize: WIKI.INI */
};

class SyncService {
public:
  enum { InfoSize = 512 };
  enum Result { Updated, Unchanged, Failed };

  SyncService( RemoteWiki &remote, PageStore &store, PageIndex &index, const SyncBuffers &buffers, const Config &config,
               Progress &progress, Error &error );
  /* The index of lang from the disk, without the network: false when there is none. */
  bool loadLocal( const char *lang );
  /* WIKI.INI from the server; a new generation brings the new index of lang. */
  Result refresh( const char *lang );
  /* The page is on the disk (downloaded when it is not). */
  bool ensurePage( const char *lang, unsigned id );
  /* Every page of lang that is not on the disk yet: how many were downloaded, -1 on an
     error. */
  int downloadAll( const char *lang );
  /* The export as last seen: the generation and the languages. */
  const WikiInfo &info() const;
  /* How many pages of lang are on the disk. */
  int pagesOnDisk( const char *lang );

private:
  Result refreshConnected( const char *lang );
  void dropStale( const char *lang );
  bool download( const char *lang, const char *name, const char *manifestText );
  bool downloadPage( const char *lang, unsigned id );
  bool loadIndex( const char *lang );

  RemoteWiki &remote_;
  PageStore &store_;
  PageIndex &index_;
  SyncBuffers buffers_;
  const Config &config_;
  Progress &progress_;
  Error &error_;
  WikiInfo info_;
};

#endif
