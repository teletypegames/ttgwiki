/*
   TTGWIKIW.EXE: the Win32 machine (Winsock, a console of its own). The test build
   (build.sh test) takes the keys from TESTKEYS and saves the screen to SCREEN.BIN when
   they run out.
*/

#include <stdlib.h>

#include "app/ttgwiki.h"
#include "lib/platform/dos/dos_file_system.h"
#include "lib/platform/stdio_text_output.h"
#include "lib/platform/win32/console_keyboard.h"
#include "lib/platform/win32/console_screen.h"
#include "lib/platform/win32/win32_clock.h"
#include "lib/testing/scripted_keyboard.h"
#include "lib/testing/screen_dump.h"
#include "lib/ui/keys.h"
#include "platform/exe_dir.h"
#include "platform/win32/winsock_network.h"

/* The screen is saved when the keys run out; then Esc until TTGWIKI ends (the reader
   needs one to get back to the contents, the contents one to quit). */
class DumpAndEsc : public ScriptEnd {
public:
  DumpAndEsc( Screen &screen, FileSystem &fs ) : screen_( screen ), fs_( fs ), dumped_( false ) {}
  unsigned scriptEnded() {
    if ( !dumped_ ) dumpScreen( screen_, fs_, "SCREEN.BIN" );
    dumped_ = true;
    return KeyEscCode;
  }

private:
  Screen &screen_;
  FileSystem &fs_;
  bool dumped_;
};

int main( int argc, char *argv[] ) {
  static WinsockNetwork network;
  static Win32Clock clock;
  static DosFileSystem fs;
  static ConsoleScreen screen;
  static StdioTextOutput out;
#ifdef TEST
  static DumpAndEsc end( screen, fs );
  static ScriptedKeyboard keyboard( getenv( "TESTKEYS" ), end );
#else
  static ConsoleKeyboard keyboard;
#endif
  static char selfDir[128];
  exeDir( argv[0], selfDir, sizeof( selfDir ) );
  Machine machine = { network, clock, fs, screen, keyboard, out, selfDir };
  return runTtgwiki( machine, argc, argv );
}
