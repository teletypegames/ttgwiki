/*
   A Screen in memory for the unit tests: text() gives a row as text. It gives the rows
   asked for (25 without asking).
*/

#ifndef DOSLIB_TESTING_MEMORY_SCREEN_H
#define DOSLIB_TESTING_MEMORY_SCREEN_H

#include "../../platform/screen.h"

class MemoryScreen : public Screen {
public:
  MemoryScreen() : cursorRow( 0 ), cursorCol( 0 ), asked_( 0 ), rows_( Rows ) {
    for ( int i = 0; i < MaxCells; i++ ) cells_[i] = 0x0720;
  }
  void setRows( int rows ) { asked_ = rows; }
  int rows() const { return rows_; }
  void begin() { rows_ = asked_ >= Rows && asked_ <= MaxRows ? asked_ : Rows; }
  void end() {}
  void put( int pos, char c, unsigned char attr ) {
    if ( pos >= 0 && pos < rows_ * Cols ) cells_[pos] = ( (unsigned)attr << 8 ) | (unsigned char)c;
  }
  unsigned cell( int pos ) const { return cells_[pos]; }
  void flush() {}
  void hideCursor() {}
  void moveCursor( int row, int col ) {
    cursorRow = row;
    cursorCol = col;
  }
  /* The row without its trailing spaces (valid until the next call). */
  const char *text( int row ) const {
    static char line[Cols + 1];
    int n = 0;
    for ( int c = 0; c < Cols; c++ ) line[c] = (char)( cells_[row * Cols + c] & 0xFF );
    for ( int c = 0; c < Cols; c++ ) {
      if ( line[c] != ' ' ) n = c + 1;
    }
    line[n] = 0;
    return line;
  }
  unsigned char attr( int row, int col ) const { return (unsigned char)( cells_[row * Cols + col] >> 8 ); }

  int cursorRow, cursorCol;

private:
  int asked_, rows_;
  unsigned cells_[MaxCells];
};

#endif
