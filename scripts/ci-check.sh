#!/bin/sh
# The fast checks before the build: the code style (clang-format), the layers
# (SOURCE/lib/tools/check_layers.py), the unit tests of the client on the host
# (SOURCE/test/unit) and the tests of the server side (server/test: the renderer and the
# exporter).
set -eu
CLANG_FORMAT="${CLANG_FORMAT:-/opt/clang-format/bin/clang-format}" sh SOURCE/lib/tools/format.sh --check SOURCE
sh SOURCE/test/unit/run.sh
python3 -m unittest discover -s server/test
