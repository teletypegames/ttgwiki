#!/bin/sh
# Run the end-to-end tests: the exporter writes a small export, the test FTP
# server shares it, TTGWIKI.EXE runs in a headless DOSBox Staging
# (SOURCE/test/run.sh) and TTGWIKIW.EXE under Wine (SOURCE/test/runw32.sh).
set -eu
DOSBOX="${DOSBOX:-/opt/dosbox-staging/dosbox}" WATCOM="${WATCOM:-/opt/watcom}" PYTHON=python3 \
  sh SOURCE/test/run.sh
PYTHON=python3 sh SOURCE/test/runw32.sh
