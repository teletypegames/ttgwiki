#include <string.h>

#include "history.h"

History::History() : count_( 0 ) {}

void History::push( const Visit &visit ) {
  if ( count_ == Size ) {
    memmove( visits_, visits_ + 1, sizeof( Visit ) * ( Size - 1 ) );
    count_--;
  }
  visits_[count_++] = visit;
}

bool History::pop( Visit &visit ) {
  if ( count_ == 0 ) return false;
  visit = visits_[--count_];
  return true;
}

void History::clear() {
  count_ = 0;
}

int History::count() const {
  return count_;
}
