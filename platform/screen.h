/*
   The 80-column text screen: 25 rows, or more when asked for with setRows() and the
   card can show them. A position is a cell index (row * 80 + column); a cell has a code
   page 437 character and a PC color attribute. What is put may only be visible after
   flush().
*/

#ifndef DOSLIB_PLATFORM_SCREEN_H
#define DOSLIB_PLATFORM_SCREEN_H

class Screen {
public:
  enum { Cols = 80, Rows = 25, TallRows = 50, MaxRows = 50, Cells = Cols * Rows, MaxCells = Cols * MaxRows };

  /* Asks the next begin() for rows rows: Rows or TallRows. 0 (the default) leaves it to
     the screen, which keeps the rows it has where it can. The screen takes what the card
     can show: a tall screen is 43 rows on an EGA, and 25 on a CGA or a monochrome card;
     rows() tells what it got. */
  virtual void setRows( int rows ) = 0;
  /* The rows after begin(): between Rows and MaxRows. */
  virtual int rows() const = 0;
  /* Takes the screen over, and gives it back as it found it (cleared, with the rows it
     had). */
  virtual void begin() = 0;
  virtual void end() = 0;
  virtual void put( int pos, char c, unsigned char attr ) = 0;
  /* attribute << 8 | character */
  virtual unsigned cell( int pos ) const = 0;
  virtual void flush() = 0;
  virtual void hideCursor() = 0;
  virtual void moveCursor( int row, int col ) = 0;
};

#endif
