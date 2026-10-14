/*
   TCP/IP on Win32: Winsock 1.1 (Windows 95 and later), non-blocking sockets.
*/

#ifndef TTGWIKI_PLATFORM_WINSOCK_NETWORK_H
#define TTGWIKI_PLATFORM_WINSOCK_NETWORK_H

#include <windows.h>
#include <winsock.h>

#include "../../lib/platform/network.h"

class WinsockNetwork : public Network {
public:
  WinsockNetwork();
  const char *start();
  void stop();
  bool resolve( const char *host, unsigned char ip[4] );
  int connect( const unsigned char ip[4], unsigned port, unsigned recvSize );
  int send( int handle, const char *buf, unsigned len );
  int receive( int handle, char *buf, unsigned len );
  void close( int handle );
  void idle();
  bool interrupted();
  void clearInterrupted();

private:
  enum { MaxHandles = 4 };

  SOCKET sockets_[MaxHandles];
  bool used_[MaxHandles];
  bool started_;
};

#endif
