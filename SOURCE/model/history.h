/*
   The pages read before the current one, for going back. The oldest is forgotten when
   it is full.
*/

#ifndef TTGWIKI_MODEL_HISTORY_H
#define TTGWIKI_MODEL_HISTORY_H

struct Visit {
  unsigned id;
  int top;      /* the first line on the screen */
  int linkLine; /* the selected link, -1 for none */
  int linkColumn;
};

class History {
public:
  enum { Size = 32 };

  History();
  void push( const Visit &visit );
  bool pop( Visit &visit );
  void clear();
  int count() const;

private:
  Visit visits_[Size];
  int count_;
};

#endif
