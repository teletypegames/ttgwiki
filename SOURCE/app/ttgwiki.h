/*
   TTGWIKI without its platform: main_dos.cpp and main_win32.cpp give it the machine,
   this puts the layers together and runs the browser or a command.
*/

#ifndef TTGWIKI_APP_TTGWIKI_H
#define TTGWIKI_APP_TTGWIKI_H

class Clock;
class FileSystem;
class Keyboard;
class Network;
class Screen;
class TextOutput;

struct Machine {
  Network &network;
  Clock &clock;
  FileSystem &fs;
  Screen &screen;
  Keyboard &keyboard;
  TextOutput &out;
  const char *selfDir; /* the folder of the executable */
};

int runTtgwiki( Machine &machine, int argc, char *argv[] );

#endif
