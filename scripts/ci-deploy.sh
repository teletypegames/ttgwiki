#!/bin/sh
# Deploy the server side (the exporter and its FTP server) to the server: copies
# compose.yaml and server/ into STACK and restarts the stack with docker compose, which
# builds the image. The export (STACK/export) and the settings (STACK/.env) stay.
#
#   STACK=/srv/stacks/ttgwiki scripts/ci-deploy.sh
#
# In the pipeline STACK is mounted at the same path as on the server, so the relative
# bind mounts of compose.yaml resolve on the server, and the server's docker socket is
# mounted too. The wiki's stack (services/wiki-pages) has to run: the exporter joins its
# network.
set -eu

STACK="${STACK:-/srv/stacks/ttgwiki}"
STACK_OWNER="${STACK_OWNER:-1000:1000}"

say() { echo "[ci-deploy] $*"; }
die() { echo "[ci-deploy] error: $*" >&2; exit 1; }

[ -d "$STACK" ] || die "no stack folder at $STACK"
cp compose.yaml "$STACK/compose.yaml.new"
rm -rf "$STACK/server.new"
cp -R server "$STACK/server.new"
rm -rf "$STACK/server.new/test"
mkdir -p "$STACK/export"
chown -R "$STACK_OWNER" "$STACK/compose.yaml.new" "$STACK/server.new" "$STACK/export" 2>/dev/null ||
  say "could not chown to $STACK_OWNER, keeping the owner"
rm -rf "$STACK/server"
mv "$STACK/server.new" "$STACK/server"
mv "$STACK/compose.yaml.new" "$STACK/compose.yaml"

cd "$STACK"
docker compose up -d --build --remove-orphans
docker compose ps
