#include <direct.h>
#include <string.h>

#include "../exe_dir.h"

void exeDir( const char *argv0, char *out, int size ) {
  if ( strchr( argv0, '\\' ) ) {
    strncpy( out, argv0, size - 1 );
    out[size - 1] = 0;
    *strrchr( out, '\\' ) = 0;
  }
  else getcwd( out, size );
  int len = strlen( out );
  if ( len > 3 && out[len - 1] == '\\' ) out[len - 1] = 0;
  if ( len == 2 ) strcat( out, "\\" );
}
