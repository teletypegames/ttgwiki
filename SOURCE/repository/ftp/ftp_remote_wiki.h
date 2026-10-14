#ifndef TTGWIKI_REPOSITORY_FTP_FTP_REMOTE_WIKI_H
#define TTGWIKI_REPOSITORY_FTP_FTP_REMOTE_WIKI_H

#include "../remote_wiki.h"
#include "../../lib/net/ftp_client.h"

class Clock;
class Error;
class Network;
class Progress;
struct Config;

/* The export as an FTP folder (Config: server, port, login, PATH). */
class FtpRemoteWiki : public RemoteWiki, private TransferObserver {
public:
  FtpRemoteWiki( Network &net, Clock &clock, const Config &config, Progress &progress, Error &error );
  bool connect();
  void disconnect();
  void shutdown();
  Fetch readText( const char *path, char *buf, unsigned size );
  bool download( const char *path, ByteSink &sink, const char *label );

private:
  bool startNetwork();
  void remotePath( const char *path, char *out ) const;
  void transferred( const char *label, unsigned long bytes );

  Network &net_;
  const Config &config_;
  Progress &progress_;
  Error &error_;
  FtpClient ftp_;
  bool netUp_;
  bool connected_;
  unsigned char ip_[4];
};

#endif
