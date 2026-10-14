/*
   The command line: messages and pages as plain text on the standard output. As the
   Progress of the operations it prints what they do (not the byte counts).
*/

#ifndef TTGWIKI_VIEW_CLI_VIEW_H
#define TTGWIKI_VIEW_CLI_VIEW_H

#include "../lib/core/progress.h"

class Error;
class TextOutput;

class CliView : public Progress {
public:
  explicit CliView( TextOutput &out );
  void report( const char *text );
  void printUsage( const char *version );
  void printMessage( const char *text );
  void printError( const Error &error );
  /* A line of a page (with its control bytes), as plain text. */
  void printPageLine( const char *line );

private:
  TextOutput &out_;
};

#endif
