#include <dos.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "types.h"
#include "trace.h"
#include "utils.h"
#include "packet.h"
#include "arp.h"
#include "udp.h"
#include "dns.h"
#include "tcp.h"
#include "tcpsockm.h"
#include "timer.h"

#include "mtcp_network.h"

static volatile bool Interrupted = false;

static void __interrupt __far ctrlBreakHandler() {
  Interrupted = true;
}

static void __interrupt __far ctrlCHandler() {}

static TcpSocket *socketOf( void *s ) {
  return (TcpSocket *)s;
}

MtcpNetwork::MtcpNetwork() : nextPort_( 0 ), up_( false ) {
  for ( int i = 0; i < MaxHandles; i++ ) sockets_[i] = NULL;
}

const char *MtcpNetwork::start() {
  if ( up_ ) return NULL;
  if ( Utils::parseEnv() != 0 ) return "mTCP is not set up (MTCPCFG)";
  if ( Utils::initStack( MaxHandles, TCP_SOCKET_RING_SIZE, ctrlBreakHandler, ctrlCHandler ) ) {
    return "Cannot start TCP/IP (packet driver loaded?)";
  }
  up_ = true;
  srand( (unsigned)time( NULL ) ^ *(uint16_t __far *)MK_FP( 0x40, 0x6C ) );
  nextPort_ = 2048 + ( rand() % 20000 );
  return NULL;
}

void MtcpNetwork::stop() {
  if ( up_ ) Utils::endStack();
  up_ = false;
}

bool MtcpNetwork::resolve( const char *host, unsigned char ip[4] ) {
  IpAddr_t addr;
  if ( Dns::resolve( (char *)host, addr, 1 ) < 0 ) return false;
  while ( !Interrupted && Dns::isQueryPending() ) {
    Utils::processPackets();
    Dns::drivePendingQuery();
  }
  if ( Dns::resolve( (char *)host, addr, 0 ) != 0 ) return false;
  memcpy( ip, addr, 4 );
  return true;
}

/* A new local port for every connection: a port from an earlier run may still be in use. */
int MtcpNetwork::connect( const unsigned char ip[4], unsigned port, unsigned recvSize ) {
  int h;
  for ( h = 0; h < MaxHandles && sockets_[h]; h++ );
  if ( h == MaxHandles ) return -1;
  IpAddr_t addr;
  memcpy( addr, ip, 4 );
  TcpSocket *s = TcpSocketMgr::getSocket();
  if ( s == NULL ) return -1;
  if ( s->setRecvBuffer( recvSize > 16384u ? 16384u : recvSize ) ) {
    TcpSocketMgr::freeSocket( s );
    return -1;
  }
  uint16_t local = nextPort_;
  nextPort_ = ( nextPort_ >= 30000 ) ? 2048 : nextPort_ + 1;
  if ( s->connect( local, addr, port, 10000 ) ) {
    TcpSocketMgr::freeSocket( s );
    return -1;
  }
  sockets_[h] = s;
  return h;
}

int MtcpNetwork::send( int h, const char *buf, unsigned len ) {
  int16_t n = socketOf( sockets_[h] )->send( (uint8_t *)buf, len );
  Utils::processPackets();
  return n < 0 ? -1 : n;
}

int MtcpNetwork::receive( int h, char *buf, unsigned len ) {
  Utils::processPackets();
  TcpSocket *s = socketOf( sockets_[h] );
  int16_t n = s->recv( (uint8_t *)buf, len );
  if ( n < 0 ) return -1;
  if ( n > 0 ) return n;
  return s->isRemoteClosed() ? Closed : 0;
}

void MtcpNetwork::close( int h ) {
  if ( h < 0 || h >= MaxHandles || sockets_[h] == NULL ) return;
  TcpSocket *s = socketOf( sockets_[h] );
  s->close();
  TcpSocketMgr::freeSocket( s );
  sockets_[h] = NULL;
}

void MtcpNetwork::idle() {
  if ( up_ ) Utils::processPackets();
}

bool MtcpNetwork::interrupted() {
  return Interrupted;
}

void MtcpNetwork::clearInterrupted() {
  Interrupted = false;
}

unsigned long MtcpClock::ms() {
  return TIMER_GET_CURRENT() * TIMER_TICK_LEN;
}

unsigned MtcpClock::ticks() {
  return *(uint16_t __far *)MK_FP( 0x40, 0x6C );
}

unsigned MtcpClock::today() {
  struct dosdate_t d;
  _dos_getdate( &d );
  return ( ( d.year - 1980 ) << 9 ) | ( d.month << 5 ) | d.day;
}
