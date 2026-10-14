#include <string.h>

#include "../core/code_page.h"
#include "../core/crc32.h"
#include "../core/error.h"
#include "../core/ini.h"
#include "../core/name_list.h"
#include "../core/str.h"
#include "../core/text.h"
#include "../testing/host/minitest.h"

TEST( cmpiUpperCasesOnlyAsciiLetters ) {
  CHECK_INT( cmpi( "alpha", "ALPHA" ), 0 );
  CHECK( cmpi( "a", "b" ) < 0 );
  CHECK( cmpi( "B", "a" ) > 0 );
  CHECK( cmpi( "_", "A" ) > 0 );
  CHECK( cmpi( "\x81", "z" ) > 0 );
}

TEST( trimCutsBlanksAndLineEnds ) {
  char s[] = "  value \t\r\n";
  CHECK_STR( trim( s ), "value" );
}

TEST( joinPathAddsOneSeparator ) {
  char out[40];
  joinLocal( out, "C:\\APPS", "HELLO" );
  CHECK_STR( out, "C:\\APPS\\HELLO" );
  joinLocal( out, "C:\\", "HELLO" );
  CHECK_STR( out, "C:\\HELLO" );
  joinRemote( out, "/", "HELLO" );
  CHECK_STR( out, "/HELLO" );
  joinLocal( out, out, "X" );
  CHECK_STR( out, "/HELLO\\X" );
}

TEST( samePathIgnoresCaseAndSlashes ) {
  CHECK( samePath( "data/readme.txt", "DATA\\README.TXT" ) );
  CHECK( !samePath( "DATA\\README", "DATA\\README.TXT" ) );
}

TEST( dosNamesAreEightDotThree ) {
  CHECK( isDosName( "HELLO.BAT" ) );
  CHECK( isDosName( "A" ) );
  CHECK( !isDosName( "TOOLONGNAME" ) );
  CHECK( !isDosName( "A.TOOL" ) );
  CHECK( !isDosName( "A B" ) );
  CHECK( !isDosName( ".HIDDEN" ) );
  CHECK( !isDosName( "A.B.C" ) );
}

TEST( executablesAreComExeBat ) {
  CHECK( isExecutable( "DOOM.EXE" ) );
  CHECK( isExecutable( "go.bat" ) );
  CHECK( isExecutable( "X.COM" ) );
  CHECK( !isExecutable( "README.TXT" ) );
  CHECK( !isExecutable( "EXE" ) );
}

TEST( numStrWritesDigits ) {
  char out[8];
  numStr( out, 0 );
  CHECK_STR( out, "0" );
  numStr( out, 65535 );
  CHECK_STR( out, "65535" );
}

TEST( iniScannerReadsKeyLines ) {
  const char *text =
      "\xEF\xBB\xBF[GAME]\r\n; comment = no\r\n# also = no\nName = \"Doom\"  \r\nnokey\r\nYEAR=1993\x1A ignored";
  IniScanner scanner( text );
  IniLine line;
  char value[20];
  int keys = 0;
  while ( scanner.next( line ) ) {
    if ( !line.isKeyLine() ) continue;
    keys++;
    line.copyValue( value, sizeof( value ) );
    if ( keys == 1 ) {
      CHECK( line.keyIs( "NAME" ) );
      CHECK_STR( value, "Doom" );
    }
    else {
      CHECK( line.keyIs( "YEAR" ) );
      CHECK_STR( value, "1993" );
    }
  }
  CHECK_INT( keys, 2 );
  CHECK( scanner.body() == text + 3 );
}

TEST( utf8BecomesCodePage437 ) {
  char s[] = "\xC3\xA1rv\xC3\xADzt\xC5\xB1r\xC5\x91 \xE2\x80\x93 \xF0\x9F\x98\x80";
  unsigned len = utf8To437( s );
  CHECK_STR( s, "\xA0rv\xA1zt\x81r\x94 - ?" );
  CHECK_INT( len, strlen( s ) );
}

TEST( codePage437BecomesUtf8 ) {
  char out[16];
  char *end = cp437ToUtf8( out, "\x81\xC4" );
  *end = 0;
  CHECK_STR( out, "\xC3\xBC\xE2\x94\x80" );
}

TEST( crc32IsTheZlibOne ) {
  Crc32 crc;
  crc.update( "1234", 4 );
  crc.update( "56789", 5 );
  CHECK( crc.value() == 0xCBF43926ul );
}

TEST( errorFormatsAndKeeps ) {
  Error error;
  CHECK( !error.isSet() );
  error.set( "Cannot %s %d", "move", 2 );
  CHECK_STR( error.text(), "Cannot move 2" );
  {
    ErrorKeeper keep( error );
    error.set( "later" );
  }
  CHECK_STR( error.text(), "Cannot move 2" );
}

TEST( nameListAddsAndWalks ) {
  char buf[16];
  NameList names( buf, sizeof( buf ) );
  CHECK( names.add( "ONE" ) );
  CHECK( names.add( "TWO" ) );
  CHECK( !names.add( "TOO-LONG-1" ) );
  CHECK_INT( names.count(), 2 );
  CHECK( names.contains( "two" ) );
  const char *n = names.first();
  CHECK_STR( n, "ONE" );
  n = names.next( n );
  CHECK_STR( n, "TWO" );
  CHECK( names.next( n ) == NULL );
}

TEST( utf8BecomesCodePage852WithTheHungarianLetters ) {
  char s[] = "\xC5\x91\xC5\xB1\xC3\x81 \xE2\x94\x80\xE2\x95\x90";
  CHECK_INT( utf8ToCodePage( s, CodePage852 ), 6 );
  CHECK_STR( s, "\x8B\xFB\xB5 \xC4\xCD" );
}

TEST( utf8BecomesCodePage437OneByteACharacter ) {
  char s[] = "\xC5\x91\xC3\x81\xE2\x80\xA2\xE2\x94\x82\xF0\x9F\x98\x80\xE2\x80\x94x";
  CHECK_INT( utf8ToCodePage( s, CodePage437 ), 7 );
  CHECK_STR( s, "\x94"
                "A\x07\xB3?-x" );
}

TEST( codePageKeepsBadBytesAndDropsTheBom ) {
  char s[] = "\xEF\xBB\xBF\x81\xC5x";
  CHECK_INT( utf8ToCodePage( s, CodePage852 ), 3 );
  CHECK_STR( s, "\x81\xC5x" );
}

TEST( foldLetterDropsTheAccent ) {
  CHECK_INT( foldLetter( 0x8B, CodePage852 ), 'O' );
  CHECK_INT( foldLetter( 0x82, CodePage437 ), 'E' );
  CHECK_INT( foldLetter( 'a', CodePage437 ), 'A' );
  CHECK_INT( foldLetter( 0xC4, CodePage852 ), 0xC4 );
  CodePage cp;
  CHECK( codePageOf( 852, cp ) && cp == CodePage852 );
  CHECK( !codePageOf( 850, cp ) );
}
