#!/bin/sh
# Build TTGWIKI.EXE (DOS) and TTGWIKIW.EXE (Win32) into the repository root
# (SOURCE/build.sh), with the Open Watcom that scripts/ci-toolchain.sh installed.
set -eu
WATCOM="${WATCOM:-/opt/watcom}" sh SOURCE/build.sh
