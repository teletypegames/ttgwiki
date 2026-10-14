/*
   TCP/IP on DOS: mTCP over a packet driver (GPLv3, see MTCP\COPYING.TXT). Its timer is
   the clock of the transfers.
*/

#ifndef TTGWIKI_PLATFORM_MTCP_NETWORK_H
#define TTGWIKI_PLATFORM_MTCP_NETWORK_H

#include "../../lib/platform/clock.h"
#include "../../lib/platform/network.h"

class MtcpNetwork : public Network {
public:
  MtcpNetwork();
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
  enum { MaxHandles = 2 };

  void *sockets_[MaxHandles];
  unsigned nextPort_;
  bool up_;
};

class MtcpClock : public Clock {
public:
  unsigned long ms();
  unsigned ticks();
  unsigned today();
};

#endif
