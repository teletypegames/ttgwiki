#!/bin/sh
# Package the built programs for a release: dist/TTGWIKI-<tag>.zip holds the TTGWIKI folder
# as it is installed on DOS (TTGWIKI.EXE, TTGWIKI.CFG, README.md, LICENSE), and
# dist/TTGWIKIW-<tag>.zip the TTGWIKIW folder for Windows (TTGWIKIW.EXE instead). The
# executables alone, dist/TTGWIKI.EXE and dist/TTGWIKIW.EXE, update an installed one.
#
#   scripts/ci-package.sh            the tag from Woodpecker (CI_COMMIT_TAG)
#   TAG=v1.2.3 scripts/ci-package.sh
#
# The tag has to match Version in SOURCE/app/version.h, which the title bar shows.
# Both must already be built (scripts/ci-build.sh, the pipeline's test step).
set -eu

TAG="${TAG:-${CI_COMMIT_TAG:-}}"
DIST="${DIST:-dist}"

say() { echo "[ci-package] $*"; }
die() { echo "[ci-package] error: $*" >&2; exit 1; }

[ -n "$TAG" ] || die "cannot work out the tag — set TAG=v1.2.3"
for exe in TTGWIKI.EXE TTGWIKIW.EXE; do
  [ -f "$exe" ] || die "$exe is not built — run scripts/ci-build.sh first"
done
SOURCE_VERSION="$(sed -n 's/^static const char Version\[\] = "\(.*\)";/\1/p' SOURCE/app/version.h)"
[ "v$SOURCE_VERSION" = "$TAG" ] || die "the tag $TAG does not match Version $SOURCE_VERSION in SOURCE/app/version.h"

rm -rf "$DIST"
mkdir -p "$DIST"
for name in TTGWIKI TTGWIKIW; do
  mkdir "$DIST/$name"
  # TTGWIKI.CFG keeps its CRLF line ends: it is copied, not regenerated.
  cp "$name.EXE" TTGWIKI.CFG README.md LICENSE "$DIST/$name/"
  (cd "$DIST" && zip -q -r "$name-$TAG.zip" "$name")
  cp "$name.EXE" "$DIST/"
  rm -rf "${DIST:?}/$name"
done

say "built:"
ls -l "$DIST"
