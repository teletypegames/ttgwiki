/*
   A line of a page as the export writes it: at most 78 characters, with control bytes
   that do not take a column:

     01h c        the style from here on (PageStyle)
     02h n 03h    a link starts, n is its number in decimal (the page's L= lines)
     04h          the link ends

   A line starts in the normal style, outside a link.
*/

#ifndef TTGWIKI_MODEL_PAGE_LINE_H
#define TTGWIKI_MODEL_PAGE_LINE_H

enum PageStyle {
  StyleNormal = '0',
  StyleHeading1 = '1',
  StyleHeading2 = '2',
  StyleHeading3 = '3',
  StyleBold = 'b',
  StyleItalic = 'i',
  StyleCode = 'c',
  StyleCodeBlock = 'p',
  StyleQuote = 'q',
  StyleRule = 'r',
  StyleTableFrame = 't',
  StyleTableHeader = 'h'
};

struct LineSegment {
  const char *text;
  int length;
  int column; /* where it starts on the line */
  char style;
  int link; /* 0: not a link */
};

/* The runs of a line that have the same style and link. */
class LineReader {
public:
  explicit LineReader( const char *line );
  bool next( LineSegment &segment );
  /* The style at the end of what was read. */
  char style() const { return style_; }

private:
  void readControls();

  const char *pos_;
  int column_;
  char style_;
  int link_;
};

/* A link on a line. */
struct LinkSpot {
  int column;
  int length;
  int link;
};

/* The first link that starts at fromColumn or later; false when there is none. */
bool findLinkAfter( const char *line, int fromColumn, LinkSpot &spot );
/* The last link that starts before beforeColumn; false when there is none. */
bool findLinkBefore( const char *line, int beforeColumn, LinkSpot &spot );
/* The line as plain text, without the control bytes, into out (size bytes). */
void plainLine( const char *line, char *out, int size );

#endif
