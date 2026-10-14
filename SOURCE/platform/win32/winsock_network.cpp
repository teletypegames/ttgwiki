#include <string.h>

#include "winsock_network.h"
#include "../../lib/platform/win32/console_break.h"

WinsockNetwork::WinsockNetwork() : started_( false ) {
  for ( int i = 0; i < MaxHandles; i++ ) used_[i] = false;
}

const char *WinsockNetwork::start() {
  if ( started_ ) return NULL;
  WSADATA wsa;
  if ( WSAStartup( MAKEWORD( 1, 1 ), &wsa ) ) return "Cannot start Winsock";
  consoleBreakCatch();
  started_ = true;
  return NULL;
}

void WinsockNetwork::stop() {
  if ( started_ ) WSACleanup();
  started_ = false;
}

bool WinsockNetwork::resolve( const char *host, unsigned char ip[4] ) {
  unsigned long addr = inet_addr( host );
  if ( addr == INADDR_NONE ) {
    struct hostent *he = gethostbyname( host );
    if ( he == NULL || he->h_length != 4 ) return false;
    memcpy( &addr, he->h_addr_list[0], 4 );
  }
  memcpy( ip, &addr, 4 );
  return true;
}

int WinsockNetwork::connect( const unsigned char ip[4], unsigned port, unsigned recvSize ) {
  int h;
  for ( h = 0; h < MaxHandles && used_[h]; h++ );
  if ( h == MaxHandles ) return -1;
  SOCKET s = socket( AF_INET, SOCK_STREAM, 0 );
  if ( s == INVALID_SOCKET ) return -1;
  int size = recvSize;
  setsockopt( s, SOL_SOCKET, SO_RCVBUF, (const char *)&size, sizeof( size ) );
  struct sockaddr_in sa;
  memset( &sa, 0, sizeof( sa ) );
  sa.sin_family = AF_INET;
  sa.sin_port = htons( (unsigned short)port );
  memcpy( &sa.sin_addr, ip, 4 );
  if ( ::connect( s, (struct sockaddr *)&sa, sizeof( sa ) ) == SOCKET_ERROR ) {
    closesocket( s );
    return -1;
  }
  unsigned long nonBlocking = 1;
  ioctlsocket( s, FIONBIO, &nonBlocking );
  sockets_[h] = s;
  used_[h] = true;
  return h;
}

int WinsockNetwork::send( int h, const char *buf, unsigned len ) {
  int n = ::send( sockets_[h], buf, len, 0 );
  if ( n == SOCKET_ERROR ) return WSAGetLastError() == WSAEWOULDBLOCK ? 0 : -1;
  return n;
}

/* Waits up to 20 ms for data, so the callers' polling loops do not spin. */
int WinsockNetwork::receive( int h, char *buf, unsigned len ) {
  fd_set readable;
  struct timeval tv = { 0, 20000 };
  FD_ZERO( &readable );
  FD_SET( sockets_[h], &readable );
  if ( select( 0, &readable, NULL, NULL, &tv ) <= 0 ) return 0;
  int n = recv( sockets_[h], buf, len, 0 );
  if ( n == 0 ) return Closed;
  if ( n == SOCKET_ERROR ) return WSAGetLastError() == WSAEWOULDBLOCK ? 0 : -1;
  return n;
}

void WinsockNetwork::close( int h ) {
  if ( h < 0 || h >= MaxHandles || !used_[h] ) return;
  closesocket( sockets_[h] );
  used_[h] = false;
}

void WinsockNetwork::idle() {
  Sleep( 10 );
}

bool WinsockNetwork::interrupted() {
  return consoleBreakRaised();
}

void WinsockNetwork::clearInterrupted() {
  consoleBreakClear();
}
