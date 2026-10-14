/*
   Test doubles of TTGWIKI's ports: the export on the server, the progress, and a
   keyboard script end that keeps the screen.
*/

#ifndef TTGWIKI_TEST_FAKES_H
#define TTGWIKI_TEST_FAKES_H

#include "../../lib/core/progress.h"
#include "../../lib/testing/scripted_keyboard.h"
#include "../../repository/remote_wiki.h"

/* The export as path -> content ("WIKI.INI", "EN/P0000001.TXT"). */
class FakeRemote : public RemoteWiki {
public:
  enum { MaxFiles = 32, ContentSize = 4096 };

  FakeRemote();
  void put( const char *path, const char *content );
  void remove( const char *path );
  /* WIKI.INI of a generation with EN and HU. */
  void putInfo( unsigned long generation );
  /* A page: its header, an empty line and the lines (each ends with \r\n). */
  void putPage( const char *lang, unsigned id, const char *title, const char *links, const char *lines );
  /* INDEX.TXT of a language from "id\tparent\tdepth\tmodified\tlocale\troute\ttitle\r\n" lines. */
  void putIndex( const char *lang, const char *lines );
  /* FILES.LST of a language with the CRC-32 and size of its files. */
  void putManifest( const char *lang );

  bool connect();
  void disconnect();
  void shutdown();
  Fetch readText( const char *path, char *buf, unsigned size );
  bool download( const char *path, ByteSink &sink, const char *label );

  int connects;
  int downloads;
  bool online;
  bool connected;

private:
  int find( const char *path ) const;

  char paths_[MaxFiles][40];
  char contents_[MaxFiles][ContentSize];
  int count_;
};

class SilentProgress : public Progress {
public:
  void report( const char * ) {}
};

class Screen;

/* Keeps the screen (as text, row by row) when the keys run out, then presses Esc. */
class KeepScreen : public ScriptEnd {
public:
  explicit KeepScreen( Screen &screen );
  unsigned scriptEnded();
  /* A row of the kept screen, without trailing spaces. */
  const char *row( int n ) const;
  bool contains( const char *text ) const;
  unsigned char attr( int row, int column ) const;

private:
  Screen &screen_;
  bool kept_;
  char rows_[25][81];
  unsigned char attrs_[25][80];
};

#endif
