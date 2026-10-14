#!/bin/sh
# End-to-end test of the Win32 build (TTGWIKIW.EXE) under Wine: the exporter writes a
# small export from a fake wiki (server/test/fixture.py), the test FTP server
# (SOURCE/test/ftpserver.py) shares it on the host as WIKI, and TTGWIKIW prints pages, downloads them all, follows a
# new export and reads offline, from the command line.
#
#   PYTHON   python with pyftpdlib (default: python3)
#   WINE     wine binary (default: wine)
set -e
cd "$(dirname "$0")"

PYTHON=${PYTHON:-python3}
WINE=${WINE:-wine}
PORT=${PORT:-2132}
export WINEDEBUG=-all WINEPREFIX="${WINEPREFIX:-/tmp/ttgwiki-wine}"

WORK=$(mktemp -d)
SERVER=
start_server() {
  FTP_ROOT="$WORK/ftp" FTP_PORT=$PORT FTP_PASSIVE=60400-60499 "$PYTHON" ftpserver.py \
    > "$WORK/ftp.log" 2>&1 &
  SERVER=$!
  sleep 1
}
stop_server() {
  kill $SERVER 2>/dev/null || true
  wait $SERVER 2>/dev/null || true
  SERVER=
}
trap 'stop_server; rm -rf "$WORK"' EXIT

mkdir -p "$WORK/ftp/WIKI"
ln -s ftp/WIKI "$WORK/export"
"$PYTHON" ../../server/test/fixture.py "$WORK/export" 1 > /dev/null
mkdir -p "$WORK/bin"
cp ../../TTGWIKIW.EXE "$WORK/bin/"
printf 'SERVER=127.0.0.1\r\nPORT=%s\r\nLANG=EN\r\nCODEPAGE=437\r\n' "$PORT" > "$WORK/bin/TTGWIKI.CFG"

# Runs the command lines given as arguments; the output, as text, goes to $WORK/out.txt.
run_w32() {
  : > "$WORK/raw.txt"
  for cmd in "$@"; do
    # shellcheck disable=SC2086
    "$WINE" "$WORK/bin/TTGWIKIW.EXE" $cmd >> "$WORK/raw.txt" 2>/dev/null || true
  done
  "$PYTHON" -c 'import sys; t = open(sys.argv[1], "rb").read().decode(sys.argv[2]).replace("\r", "")
print("".join(l + "\n" for l in t.split("\n") if not l.startswith(("Resolving ", "Connecting ", "Reading ", "Downloading "))), end="")' \
    "$WORK/raw.txt" "${CODEPAGE:-cp437}" > "$WORK/out.txt"
}

FAILED=0
ok() { echo "ok    $1"; }
fail() { echo "FAIL  $1"; FAILED=1; }
check() {
  if grep -q -E "$1" "$WORK/out.txt"; then ok "$2"; else fail "$2"; fi
}

start_server

echo "--- print and download"
run_w32 "SHOW /dos" "SYNC" "SYNC"
cat "$WORK/out.txt"
check '^DOS page version 1\.$' "SHOW prints the page"
check '^│ Tool  │ Does │$' "SHOW prints a table"
check '^EN: export #1, 3 pages, 2 downloaded$' "SYNC downloads the pages not yet on the disk"
check '^EN: export #1, 3 pages, 0 downloaded$' "a second SYNC downloads nothing"
cmp -s "$WORK/bin/CACHE/EN/P0000002.TXT" "$WORK/export/EN/P0000002.TXT" && ok "a page is byte exact" ||
  fail "a page is byte exact"

echo "--- a new export, Hungarian in code page 852"
"$PYTHON" ../../server/test/fixture.py "$WORK/export" 2 > /dev/null
printf 'SERVER=127.0.0.1\r\nPORT=%s\r\nLANG=EN\r\nCODEPAGE=852\r\n' "$PORT" > "$WORK/bin/TTGWIKI.CFG"
CODEPAGE=cp852 run_w32 "SHOW /dos" "SHOW / HU"
cat "$WORK/out.txt"
check '^DOS page version 2\.$' "the changed page comes from the new export"
check '^Üdvözlünk, őszült fejű róka\.$' "SHOW / HU in code page 852"

echo "--- offline"
stop_server
run_w32 "SHOW /dos" "SHOW /dos/gmenu HU"
cat "$WORK/out.txt"
check '^Offline \(.*\): the pages on the disk$' "without the server SHOW says it is offline"
check '^DOS page version 2\.$' "a page on the disk is shown offline"
check '^Error: ' "a page that is not on the disk cannot be shown offline"

[ $FAILED = 0 ] && echo "All Win32 tests passed." || { echo "Some Win32 tests FAILED."; exit 1; }
