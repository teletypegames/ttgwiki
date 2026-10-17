#!/bin/sh
# Install what the pipeline needs into a Debian image: Open Watcom v2 (the compiler, for
# DOS and Win32), DOSBox Staging (the DOS tests run TTGWIKI in it, with its NE2000
# emulation), Wine (the Win32 tests), Python with pyftpdlib (the tests' FTP server), g++
# (the unit tests) and clang-format (the code style). The downloads are pinned releases.
#
#   OW_RELEASE       Open Watcom release tag      (default: 2026-10-01-Build)
#   DOSBOX_VERSION   DOSBox Staging release tag   (default: v0.83.0)
#
# Open Watcom goes to /opt/watcom (only the Linux binaries, headers and the 16-bit and
# 32-bit libraries),
# DOSBox Staging to /opt/dosbox-staging.
#
# The pipeline's image (git.teletypegames.org/build/dos-ci:bookworm, built by
# services/teletypegames ci/dos-bookworm.Dockerfile) has all of this preinstalled and lists
# its versions in /opt/dos-ci/versions; when they are the ones pinned here, nothing is installed.
set -eu

CLANG_FORMAT_VERSION="${CLANG_FORMAT_VERSION:-18.1.8}"

OW_RELEASE="${OW_RELEASE:-2026-10-01-Build}"
DOSBOX_VERSION="${DOSBOX_VERSION:-v0.83.0}"

say() { echo "[ci-toolchain] $*"; }

preinstalled() {
  [ -r /opt/dos-ci/versions ] || return 1
  for pin in "$@"; do grep -qx "$pin" /opt/dos-ci/versions || return 1; done
}
if preinstalled "OW_RELEASE=$OW_RELEASE" "DOSBOX_VERSION=$DOSBOX_VERSION" \
  "CLANG_FORMAT_VERSION=$CLANG_FORMAT_VERSION"; then
  say "preinstalled in the CI image: $(tr '\n' ' ' < /opt/dos-ci/versions)"
  exit 0
fi

dpkg --add-architecture i386
apt-get update
apt-get install -y --no-install-recommends ca-certificates curl xz-utils zip unzip jq \
  python3 python3-pyftpdlib python3-venv g++ perl libasound2 libgl1 libglib2.0-0 libxkbcommon0 \
  wine wine32 >/dev/null

say "Open Watcom $OW_RELEASE"
mkdir -p /opt/watcom
curl -fsSL "https://github.com/open-watcom/open-watcom-v2/releases/download/$OW_RELEASE/ow-snapshot.tar.xz" |
  tar -xJ -C /opt/watcom ./binl64 ./h ./lib286 ./lib386

say "DOSBox Staging $DOSBOX_VERSION"
mkdir -p /opt/dosbox-staging
curl -fsSL "https://github.com/dosbox-staging/dosbox-staging/releases/download/$DOSBOX_VERSION/dosbox-staging-linux-x86_64-$DOSBOX_VERSION.tar.xz" |
  tar -xJ -C /opt/dosbox-staging --strip-components=1
/opt/dosbox-staging/dosbox --version | head -1
wine --version

say "clang-format $CLANG_FORMAT_VERSION"
python3 -m venv /opt/clang-format
/opt/clang-format/bin/pip install --quiet "clang-format==$CLANG_FORMAT_VERSION"
