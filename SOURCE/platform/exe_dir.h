#ifndef TTGWIKI_PLATFORM_EXE_DIR_H
#define TTGWIKI_PLATFORM_EXE_DIR_H

/* The folder of the running executable, without a trailing backslash (but "C:\"). */
void exeDir( const char *argv0, char *out, int size );

#endif
