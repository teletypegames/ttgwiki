#ifndef DOSLIB_TESTING_SCREEN_DUMP_H
#define DOSLIB_TESTING_SCREEN_DUMP_H

class FileSystem;
class Screen;

/* The screen as character / attribute pairs, 80 a row, rows() rows (SCREEN.BIN of the
   tests). */
bool dumpScreen( Screen &screen, FileSystem &fs, const char *path );

#endif
