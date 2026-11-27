#include <i86.h>

#include "dos_screen.h"

DosScreen::DosScreen( Mode mode )
  : mode_( mode ), mono_( false ), changed_( false ), asked_( 0 ), rows_( Rows ), startRows_( Rows ),
    video_( (unsigned short __far *)MK_FP( 0xB800, 0 ) ) {}

static int biosRows() {
  int rows = *(unsigned char __far *)MK_FP( 0x40, 0x84 ) + 1;
  if ( rows < Screen::Rows || rows > Screen::MaxRows ) return Screen::Rows;
  return rows;
}

static bool hasEga() {
  union REGS r;
  r.h.ah = 0x12;
  r.h.bl = 0x10;
  int86( 0x10, &r, &r );
  return r.h.bl != 0x10;
}

void DosScreen::setTextMode( int rows ) {
  union REGS r;
  r.w.ax = 3;
  int86( 0x10, &r, &r );
  if ( rows > Rows && hasEga() ) {
    r.w.ax = 0x1112;
    r.h.bl = 0;
    int86( 0x10, &r, &r );
  }
  rows_ = biosRows();
}

void DosScreen::setRows( int rows ) {
  asked_ = rows;
}

int DosScreen::rows() const {
  return rows_;
}

void DosScreen::begin() {
  union REGS r;
  r.h.ah = 0x0F;
  int86( 0x10, &r, &r );
  mono_ = ( r.h.al == 7 );
  startRows_ = mono_ ? Rows : biosRows();
  changed_ = false;
  if ( mode_ == SetMode ) {
    setTextMode( asked_ ? asked_ : Rows );
    return;
  }
  video_ = (unsigned short __far *)MK_FP( mono_ ? 0xB000 : 0xB800, 0 );
  rows_ = startRows_;
  if ( asked_ && asked_ != startRows_ && !mono_ ) {
    setTextMode( asked_ );
    changed_ = true;
  }
}

void DosScreen::end() {
  if ( mode_ == SetMode || changed_ ) {
    setTextMode( startRows_ );
    changed_ = false;
    return;
  }
  for ( int pos = 0; pos < rows_ * Cols; pos++ ) put( pos, ' ', 0x07 );
  union REGS r;
  r.h.ah = 0x01;
  r.w.cx = mono_ ? 0x0B0C : 0x0607;
  int86( 0x10, &r, &r );
  moveCursor( 0, 0 );
}

void DosScreen::put( int pos, char c, unsigned char attr ) {
  if ( pos < 0 || pos >= rows_ * Cols ) return;
  if ( mono_ ) attr = ( ( attr & 0xF0 ) && attr != 0x0F ) ? 0x70 : 0x07;
  video_[pos] = ( (unsigned short)attr << 8 ) | (unsigned char)c;
}

unsigned DosScreen::cell( int pos ) const {
  return video_[pos];
}

void DosScreen::flush() {}

void DosScreen::hideCursor() {
  union REGS r;
  r.h.ah = 0x01;
  r.w.cx = 0x2000;
  int86( 0x10, &r, &r );
}

void DosScreen::moveCursor( int row, int col ) {
  union REGS r;
  r.h.ah = 0x02;
  r.h.bh = 0;
  r.h.dh = (unsigned char)row;
  r.h.dl = (unsigned char)col;
  int86( 0x10, &r, &r );
}
