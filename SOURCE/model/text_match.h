#ifndef TTGWIKI_MODEL_TEXT_MATCH_H
#define TTGWIKI_MODEL_TEXT_MATCH_H

#include "../lib/core/code_page.h"

/* query is in text, ignoring case and accents (both in the code page cp). */
bool containsFolded( const char *text, const char *query, CodePage cp );

#endif
