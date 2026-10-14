/*
   The export on the server. Paths are relative to its root, with '/' ("WIKI.INI",
   "EN/FILES.LST"). Failures go into the Error.
*/

#ifndef TTGWIKI_REPOSITORY_REMOTE_WIKI_H
#define TTGWIKI_REPOSITORY_REMOTE_WIKI_H

class ByteSink;

class RemoteWiki {
public:
  enum Fetch { Fetched, Missing, Failed };

  /* A session with the server (the network starts with the first one). */
  virtual bool connect() = 0;
  virtual void disconnect() = 0;
  /* Stops the network at the end of the program. */
  virtual void shutdown() = 0;

  /* A small text file, terminated. */
  virtual Fetch readText( const char *path, char *buf, unsigned size ) = 0;
  /* label names the download in the progress */
  virtual bool download( const char *path, ByteSink &sink, const char *label ) = 0;
};

#endif
