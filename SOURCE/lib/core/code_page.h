/*
   The DOS code pages of the screen: 437 (the PC's own) and 852 (Central European, with
   the Hungarian letters). A text in UTF-8 becomes one byte for every character, so it
   keeps its width in columns.
*/

#ifndef DOSLIB_CORE_CODE_PAGE_H
#define DOSLIB_CORE_CODE_PAGE_H

enum CodePage { CodePage437 = 437, CodePage852 = 852 };

/* The code page of a number (437 or 852); false for any other. */
bool codePageOf( unsigned number, CodePage &cp );
/* UTF-8 -> the code page in place (s is terminated): the new length. Every character
   becomes one byte: one the code page lacks becomes its base letter, a near letter
   (o with double acute -> o with umlaut in 437), a line of the box drawing or '?'.
   Bytes that do not form a valid UTF-8 sequence are kept; a byte order mark is dropped. */
unsigned utf8ToCodePage( char *s, CodePage cp );
/* A character of the code page as an upper-case letter without its accent (for
   searching); anything but a letter stays as it is. */
unsigned char foldLetter( unsigned char c, CodePage cp );

#endif
