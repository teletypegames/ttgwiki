#include <string.h>

#include "cli_view.h"
#include "../lib/core/error.h"
#include "../lib/platform/text_output.h"
#include "../model/page_line.h"

CliView::CliView( TextOutput &out ) : out_( out ) {}

void CliView::report( const char *text ) {
  if ( strstr( text, " bytes" ) == NULL ) printTo( out_, "%s\n", text );
}

void CliView::printUsage( const char *version ) {
  printTo( out_, "TTGWIKI %s - the Teletype Games wiki, read on DOS\n\n", version );
  printTo( out_, "TTGWIKI                   browse the wiki\n" );
  printTo( out_, "TTGWIKI SYNC [lang]       download every page, to read them offline\n" );
  printTo( out_, "TTGWIKI SHOW route [lang] print a page (TTGWIKI SHOW /dos/gmenu > PRN)\n\n" );
  printTo( out_, "The settings are in TTGWIKI.CFG next to TTGWIKI.EXE.\n" );
}

void CliView::printMessage( const char *text ) {
  printTo( out_, "%s\n", text );
}

void CliView::printError( const Error &error ) {
  printTo( out_, "Error: %s\n", error.text() );
}

/* The screen shows a glyph for the control characters (a bullet, arrows); a file or a
   printer would take them as commands. */
static char printable( char c ) {
  switch ( c ) {
    case 0x07: return '*';
    case 0x10:
    case 0x1A: return '>';
    case 0x11:
    case 0x1B: return '<';
    case 0x18:
    case 0x1E: return '^';
    case 0x19:
    case 0x1F: return 'v';
    case 0x1D: return '-';
    default: return (unsigned char)c < 0x20 ? ' ' : c;
  }
}

void CliView::printPageLine( const char *line ) {
  char plain[256];
  plainLine( line, plain, sizeof( plain ) );
  for ( char *p = plain; *p; p++ ) *p = printable( *p );
  int len = strlen( plain );
  while ( len && plain[len - 1] == ' ' ) plain[--len] = 0;
  printTo( out_, "%s\n", plain );
}
