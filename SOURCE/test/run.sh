#!/bin/sh
# End-to-end test: the exporter writes a small export from a fake wiki
# (server/test/fixture.py), the FTP server of the server side shares it on the host, and
# TTGWIKI runs in a headless DOSBox Staging (NE2000 + mTCP): it prints pages, downloads
# them all, follows a new export, reads offline, and browses with scripted keys.
#
#   PYTHON   python with pyftpdlib (default: python3)
#   DOSBOX   DOSBox Staging binary (default: the macOS app)
#   MTCP     folder with NE2000.COM and DHCP.EXE (default: net)
set -e
cd "$(dirname "$0")"
HERE=$(pwd)

PYTHON=${PYTHON:-python3}
../build.sh test > /dev/null
DOSBOX=${DOSBOX:-"/Applications/DOSBox Staging.app/Contents/MacOS/dosbox"}
MTCP=${MTCP:-$HERE/net}
PORT=${PORT:-2131}

WORK=$(mktemp -d)
SERVER=
trap 'stop_server; rm -rf "$WORK"' EXIT

"$PYTHON" ../../server/test/fixture.py "$WORK/export" 1 > /dev/null
mkdir -p "$WORK/c/TTGWIKI" "$WORK/c/MTCP"
cp ../../TTGWIKI.EXE "$WORK/c/TTGWIKI/"
cp ../build/TTGWIKI.EXE "$WORK/c/TTGWIKI/TTGWIKIT.EXE"
cp "$MTCP/NE2000.COM" "$MTCP/DHCP.EXE" "$WORK/c/MTCP/"
printf 'PACKETINT 0x60\r\n' > "$WORK/c/MTCP/MTCP.CFG"
printf 'SERVER=10.0.2.2\r\nPORT=%s\r\nLANG=EN\r\nCODEPAGE=437\r\n' "$PORT" > "$WORK/c/TTGWIKI/TTGWIKI.CFG"

start_server() {
  FTP_ROOT="$WORK/export" FTP_PORT=$PORT FTP_PASSIVE=60200-60299 "$PYTHON" ../../server/ftpserver.py \
    > "$WORK/ftp.log" 2>&1 &
  SERVER=$!
  sleep 1
}
stop_server() {
  kill $SERVER 2>/dev/null || true
  wait $SERVER 2>/dev/null || true
  SERVER=
}

# Runs the TTGWIKI command lines given as arguments, the output goes to $WORK/c/OUT.TXT
# (and as text to $WORK/out.txt). With KEYS set, the test build runs the browser with
# those keys first, and the screen it saved is turned into text in $WORK/screen.txt.
run_dos() {
  {
    printf '[dosbox]\nmemsize = 16\n[cpu]\ncycles = max\n[ethernet]\nne2000 = on\n[autoexec]\n'
    printf 'mount c: "%s"\nc:\n' "$WORK/c"
    printf 'C:\\MTCP\\NE2000.COM 0x60 3 0x300 > NUL\nSET MTCPCFG=C:\\MTCP\\MTCP.CFG\nC:\\MTCP\\DHCP.EXE > NUL\n'
    printf 'ECHO.> C:\\OUT.TXT\n'
    if [ -n "$KEYS" ]; then
      printf 'SET TESTKEYS=%s\nC:\\TTGWIKI\\TTGWIKIT\n' "$KEYS"
    fi
    for cmd in "$@"; do printf 'C:\\TTGWIKI\\TTGWIKI %s >> C:\\OUT.TXT\n' "$cmd"; done
    printf 'exit\n'
  } > "$WORK/test.conf"
  rm -f "$WORK/c/SCREEN.BIN"
  SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy perl -e 'alarm 180; exec @ARGV' \
    "$DOSBOX" --noprimaryconf --nolocalconf -conf "$WORK/test.conf" > "$WORK/dosbox.log" 2>&1
  "$PYTHON" -c 'import sys; t = open(sys.argv[1], "rb").read().decode("cp437").replace("\r", "")
print("".join(l + "\n" for l in t.split("\n") if not l.startswith(("Resolving ", "Connecting ", "Reading ", "Downloading "))), end="")' \
    "$WORK/c/OUT.TXT" > "$WORK/out.txt"
  if [ -n "$KEYS" ]; then
    "$PYTHON" -c 'import sys; d = open(sys.argv[1], "rb").read()
for y in range(25): print("".join(chr(c) if 32 <= c < 127 else "~" if c == 26 else " " for c in d[y * 160:y * 160 + 160:2]).rstrip())' \
      "$WORK/c/SCREEN.BIN" > "$WORK/screen.txt"
  fi
}

FAILED=0
ok() { echo "ok    $1"; }
fail() { echo "FAIL  $1"; FAILED=1; }
check() {
  if grep -q -E "$1" "$WORK/out.txt"; then ok "$2"; else fail "$2"; fi
}
check_screen() {
  if grep -q -E "$1" "$WORK/screen.txt"; then ok "$2"; else fail "$2"; fi
}

start_server

echo "--- print and download"
run_dos "SHOW /dos" "SHOW /nothing" "SYNC" "SYNC"
cat "$WORK/out.txt"
check '^DOS page version 1\.$' "SHOW prints the page"
check '^│ Tool  │ Does │$' "SHOW prints a table"
check '^Error: No page /nothing$' "SHOW of a missing page fails"
check '^EN: export #1, 3 pages, 2 downloaded$' "SYNC downloads the pages not yet on the disk"
check '^EN: export #1, 3 pages, 0 downloaded$' "a second SYNC downloads nothing"
ls "$WORK/c/TTGWIKI/CACHE/EN"
[ -f "$WORK/c/TTGWIKI/CACHE/EN/P0000003.TXT" ] && ok "the pages are in CACHE\\EN" || fail "the pages are in CACHE\\EN"
cmp -s "$WORK/c/TTGWIKI/CACHE/EN/P0000002.TXT" "$WORK/export/EN/P0000002.TXT" && ok "a page is byte exact" ||
  fail "a page is byte exact"
if grep -rq "never exported" "$WORK/export"; then fail "the private page is not in the export"; else
  ok "the private page is not in the export"; fi

echo "--- a new export"
"$PYTHON" ../../server/test/fixture.py "$WORK/export" 2 > /dev/null
run_dos "SHOW /dos" "SHOW /dos/gmenu"
cat "$WORK/out.txt"
check '^DOS page version 2\.$' "the changed page comes from the new export"
check '^ GMENU\.BAT' "an unchanged page is still there"

echo "--- Hungarian, code page 852"
printf 'SERVER=10.0.2.2\r\nPORT=%s\r\nLANG=EN\r\nCODEPAGE=852\r\n' "$PORT" > "$WORK/c/TTGWIKI/TTGWIKI.CFG"
run_dos "SHOW / HU"
if "$PYTHON" -c 'import sys; t = open(sys.argv[1], "rb").read().decode("cp852"); sys.exit(0 if "Üdvözlünk, őszült fejű róka." in t else 1)' \
  "$WORK/c/OUT.TXT"; then ok "SHOW / HU in code page 852"; else fail "SHOW / HU in code page 852"; fi

echo "--- offline"
stop_server
run_dos "SHOW /dos" "SHOW /dos HU"
cat "$WORK/out.txt"
check '^Offline \(.*\): the pages on the disk$' "without the server SHOW says it is offline"
check '^DOS page version 2\.$' "a page on the disk is shown offline"
check '^Error: ' "a page that is not on the disk cannot be shown offline"
start_server

echo "--- browser"
printf 'SERVER=10.0.2.2\r\nPORT=%s\r\nLANG=EN\r\nCODEPAGE=437\r\n' "$PORT" > "$WORK/c/TTGWIKI/TTGWIKI.CFG"
KEYS="4700" run_dos
cat "$WORK/screen.txt"
check_screen '^ TTGWIKI [0-9.]+ - Teletype Games Wiki +EN  #2$' "the contents show the title and the export"
check_screen '^   DOS tools +1970-01-01$' "the contents show the tree"
check_screen '^ 3 pages, export #2' "the status line tells the pages"
KEYS="1C0D 0F09" run_dos
cat "$WORK/screen.txt"
check_screen '^ Home +EN  1-4/4$' "Enter reads the page"
check_screen '^ ~ DOS tools$' "Tab selects the first link and tells where it goes"
KEYS="1C0D 0F09 1C0D 0F09 1C0D" run_dos
check_screen '^ GMENU +EN' "Enter follows the links"
KEYS="1C0D 0F09 1C0D 0E08" run_dos
check_screen '^ Home +EN' "Backspace goes back"
KEYS="2267 226D 1C0D" run_dos
check_screen '^ GMENU +EN' "typing searches, Enter reads what it found"
KEYS="1C0D 3C00" run_dos
check_screen '^ Kezd.lap +HU' "F2 shows the page in Hungarian"

[ $FAILED = 0 ] && echo "All tests passed." || { echo "Some tests FAILED."; exit 1; }
