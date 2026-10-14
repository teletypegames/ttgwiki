#!/bin/sh
# Publish the built programs to the DOSINST program store
# (ftp://ftp.teletypegames.org/APPS), so DOSINST installs and updates TTGWIKI: the
# TTGWIKI folder (TTGWIKI.EXE, PLATFORM=DOS) and
# the TTGWIKIW folder (TTGWIKIW.EXE, PLATFORM=WIN32), each with README.md, LICENSE, an
# INFO.INI that has the version of the tag, and FILES.LST (DOSINST's server/mkfiles.sh
# format) that DOSINST checks the downloads against. TTGWIKI.CFG is left out: TTGWIKI
# works without it, and an update never touches the settings of an installed TTGWIKI.
# The pages TTGWIKI downloaded (its CACHE folder) stay too: DOSINST keeps the files it
# did not install.
#
#   scripts/ci-store.sh              the tag from Woodpecker (CI_COMMIT_TAG)
#   TAG=v1.2.3 STORE=/tmp/store scripts/ci-store.sh
#
# STORE is the store's programs folder; in the pipeline /srv/stacks/ftp-server/root/APPS of
# the server is mounted there. Each new folder is put together next to the old one and
# swapped in with two renames, so a download never sees it half written. The files get
# STORE_OWNER (the server's tasi user), so the store can be managed without sudo.
set -eu

TAG="${TAG:-${CI_COMMIT_TAG:-}}"
STORE="${STORE:-/store}"
STORE_OWNER="${STORE_OWNER:-1000:1000}"

say() { echo "[ci-store] $*"; }
die() { echo "[ci-store] error: $*" >&2; exit 1; }

[ -n "$TAG" ] || die "cannot work out the tag — set TAG=v1.2.3"
[ -d "$STORE" ] || die "no store folder at $STORE"
VERSION="${TAG#v}"

# FILES.LST: "crc32 size path" per line, the CRC from the gzip trailer.
files_list() {
  (cd "$1" && find . -type f ! -name FILES.LST ! -name FILES.LST.new | sed 's|^\./||' | sort |
    while IFS= read -r f; do
      crc=$(gzip -c "$f" | tail -c 8 | od -An -tx4 -N4 | tr -d ' \n')
      size=$(wc -c < "$f" | tr -d ' ')
      printf '%s %s %s\r\n' "$crc" "$size" "$f"
    done) > "$1/FILES.LST.new"
  mv "$1/FILES.LST.new" "$1/FILES.LST"
}

# publish FOLDER NAME PLATFORM DESCRIPTION
publish() {
  name="$1"
  [ -f "$name.EXE" ] || die "$name.EXE is not built — run scripts/ci-build.sh first"
  new="$STORE/.$name.new"
  old="$STORE/.$name.old"
  rm -rf "$new" "$old"
  mkdir "$new"
  cp "$name.EXE" README.md LICENSE "$new/"
  printf '[program]\r\nname=%s\r\ndescription=%s\r\nversion=%s\r\nplatform=%s\r\n' \
    "$2" "$4" "$VERSION" "$3" > "$new/INFO.INI"
  files_list "$new"
  chown -R "$STORE_OWNER" "$new" 2>/dev/null || say "could not chown to $STORE_OWNER, keeping the owner"
  chmod -R a+rX "$new"
  if [ -d "$STORE/$name" ]; then
    mv "$STORE/$name" "$old"
  fi
  mv "$new" "$STORE/$name"
  rm -rf "$old"
  say "published $name $VERSION:"
  ls -l "$STORE/$name"
}

publish TTGWIKI "TTGWIKI" DOS "The Teletype Games wiki on DOS: browse, search and print its pages, or download them all to read offline. Needs mTCP (a packet driver and MTCPCFG)."
publish TTGWIKIW "TTGWIKI for Windows" WIN32 "TTGWIKI for Windows 95 and later: the Teletype Games wiki in a console window, with Winsock."
