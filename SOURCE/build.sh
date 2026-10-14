#!/bin/sh
# Builds ../TTGWIKI.EXE (DOS) and ../TTGWIKIW.EXE (Win32) with Open Watcom v2.
# WATCOM points to the Open Watcom folder (default: ~/.local/opt/watcom).
# ./build.sh test builds the test versions (scripted keys, see test/run.sh) into build/.
set -e
cd "$(dirname "$0")"
. lib/tools/watcom.sh

OBJ=build
OUT=../TTGWIKI.EXE
OUTW=../TTGWIKIW.EXE
DEFS=
if [ "$1" = test ]; then
  OUT=$OBJ/TTGWIKI.EXE
  OUTW=$OBJ/TTGWIKIW.EXE
  DEFS=-dTEST
fi
rm -rf "$OBJ"
mkdir -p "$OBJ"

COMMON="lib/core/*.cpp lib/net/*.cpp lib/ui/*.cpp lib/testing/*.cpp
        lib/platform/file_system.cpp lib/platform/file_tree.cpp lib/platform/text_output.cpp lib/platform/stdio_text_output.cpp
        lib/platform/dos/dos_file_system.cpp
        model/*.cpp repository/*/*.cpp service/*.cpp controller/*.cpp view/*.cpp app/*.cpp"
DOS="lib/platform/dos/dos_screen.cpp lib/platform/dos/bios_keyboard.cpp platform/dos/exe_dir.cpp"
WIN32="lib/platform/win32/*.cpp platform/win32/*.cpp"
COMMON=$(echo $COMMON)
DOS=$(echo $DOS)
WIN32=$(echo $WIN32)

# mTCP includes its headers in mixed case ("types.h", "Eth.h", "Global.Cfg") while the
# files are upper case. A case-sensitive file system (Linux) needs a link for every name
# the sources use; on macOS the links are harmless.
INC=$OBJ/inc
mkdir -p "$INC"
for name in $(cat MTCP/TCPLIB/*.CPP MTCP/TCPINC/* platform/dos/mtcp_network.cpp STACK.CFG |
              sed -n 's/^ *# *include *"\([^"]*\)".*/\1/p' | sort -u) stack.cfg; do
  file=$(find MTCP/TCPINC . -maxdepth 1 -type f -iname "$name" | head -1)
  [ -n "$file" ] && ln -sf "$(pwd)/$file" "$INC/$name"
done

FLAGS16="-0 -ml -oh -ok -os -s -ei -zp2 -zm -we -zq -i=$WATCOM/h"
MTCP="$FLAGS16 -i=$INC -i=MTCP/TCPINC -i=. -dCFG_H=\"stack.cfg\""
TCPLIB="PACKET ARP ETH IP TCP TCPSOCKM UDP UTILS DNS TIMER TRACE"
for f in $TCPLIB; do
  wpp MTCP/TCPLIB/$f.CPP $MTCP -zpw -fo="$OBJ/$f.obj"
done
wasm -0 -ml -zq MTCP/TCPLIB/IPASM.ASM -fo="$OBJ/IPASM.obj"
compile wpp "$MTCP" platform/dos/mtcp_network.cpp
compile wpp "$FLAGS16" $COMMON $DOS
compile wpp "$FLAGS16 $DEFS" main_dos.cpp
MTCPOBJS=$(for f in $TCPLIB IPASM; do printf '%s/%s.obj,' "$OBJ" "$f"; done)
wlink system dos option quiet option eliminate option stack=12288 name "$OUT" \
  file "$MTCPOBJS$(objs platform/dos/mtcp_network.cpp $COMMON $DOS main_dos.cpp)"
rm -f "$OBJ"/*.obj

# Win32: the same layers with the Win32 platform, a console program for Windows 95 and later.
FLAGS32="-bt=nt -3r -mf -oh -ok -os -s -ei -zp4 -zm -we -zq -i=$WATCOM/h -i=$WATCOM/h/nt"
compile wpp386 "$FLAGS32" $COMMON $WIN32
compile wpp386 "$FLAGS32 $DEFS" main_win32.cpp
wlink system nt option quiet option eliminate name "$OUTW" file "$(objs $COMMON $WIN32 main_win32.cpp)" library wsock32
rm -rf "$OBJ"/*.obj "$INC"
ls -l "$OUT" "$OUTW"
