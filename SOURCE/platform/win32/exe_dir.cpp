#include <windows.h>
#include <string.h>

#include "../exe_dir.h"

void exeDir( const char *argv0, char *out, int size ) {
  if ( GetModuleFileNameA( NULL, out, size ) == 0 ) {
    strncpy( out, argv0, size - 1 );
    out[size - 1] = 0;
  }
  char *slash = strrchr( out, '\\' );
  if ( slash ) *slash = 0;
  else GetCurrentDirectoryA( size, out );
  if ( strlen( out ) == 2 ) strcat( out, "\\" );
}
