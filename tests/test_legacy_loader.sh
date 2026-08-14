#!/usr/bin/env bash
set -euo pipefail

appimage=${1:?usage: test_legacy_loader.sh LEGACY.APPIMAGE}
project_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
log=$(mktemp)
trap 'rm -f "$log"' EXIT

set +e
LD_LIBRARY_PATH="$project_root/build${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" \
LIBFUSE2_COMPAT_LOG=1 \
timeout --signal=TERM 15 "$appimage" --help > /dev/null 2>"$log"
status=$?
set -e

if ! grep -q 'libfuse2-compat: using backend libfuse3.so' "$log"; then
    cat "$log" >&2
    printf 'legacy AppImage did not load the compatibility bridge\n' >&2
    exit 1
fi

printf 'legacy AppImage loader test: ok (payload status %d)\n' "$status"
