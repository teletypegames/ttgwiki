/*
   TTGWIKI.CFG next to the executable: the server, the language and the code page.
*/

#ifndef TTGWIKI_MODEL_CONFIG_H
#define TTGWIKI_MODEL_CONFIG_H

#include "../lib/core/code_page.h"

static const char ConfigFile[] = "TTGWIKI.CFG";

struct Config {
  char server[80];
  unsigned port;
  char user[40];
  char password[40];
  char root[80]; /* the export's folder on the server */
  char lang[4];  /* EN, HU: the language folder of the export */
  CodePage codePage;
  char cache[64]; /* where the pages are kept; empty: CACHE next to the executable */

  Config();
  /* SERVER= ... lines; the defaults stay for what is missing. false: a CODEPAGE that is
     not 437 or 852. */
  bool read( const char *ini );
};

#endif
