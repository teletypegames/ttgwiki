/*
   The text screen in video memory. SetMode: begin() sets mode 3 (80x25 color, which also
   clears it), with the 8x8 font when a tall screen was asked for; end() sets mode 3
   again with the rows the screen had before. KeepMode: the screen stays as it is (a monochrome card works too: the
   colored bars become inverse) unless other rows were asked for; end() clears it and
   puts the cursor back, or sets the mode back when begin() changed it. The rows come
   from the BIOS data area.
*/

#ifndef DOSLIB_PLATFORM_DOS_SCREEN_H
#define DOSLIB_PLATFORM_DOS_SCREEN_H

#include "../screen.h"

class DosScreen : public Screen {
public:
  enum Mode { SetMode, KeepMode };

  explicit DosScreen( Mode mode );
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
  void setTextMode( int rows );

  Mode mode_;
  bool mono_;
  bool changed_;
  int asked_;
  int rows_;
  int startRows_;
  unsigned short __far *video_;
};

#endif
