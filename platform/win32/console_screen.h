#ifndef DOSLIB_PLATFORM_CONSOLE_SCREEN_H
#define DOSLIB_PLATFORM_CONSOLE_SCREEN_H

#include <windows.h>

#include "../screen.h"

/* The screen in a console screen buffer of its own, which end() gives back: 80x25, or
   80x50 when it was asked for and the console window can be that tall. */
class ConsoleScreen : public Screen {
public:
  ConsoleScreen();
  void setRows( int rows );
  int rows() const;
  void begin();
  void end();
  void put( int pos, char c, unsigned char attr );
  unsigned cell( int pos ) const;
  void flush();
  void hideCursor();
  void moveCursor( int row, int col );

private:
  HANDLE screen_;
  HANDLE oldScreen_;
  CHAR_INFO cells_[MaxCells];
  int dirtyFrom_, dirtyTo_;
  int asked_;
  int rows_;
};

#endif
