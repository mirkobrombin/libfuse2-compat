#!/usr/bin/env bash
set -euo pipefail

if (( $# < 1 )); then
    printf 'usage: %s LEGACY.APPIMAGE [PAYLOAD_ARGS...]\n' "$0" >&2
    exit 2
fi

appimage=$(realpath "$1")
shift

if [[ ! -f $appimage || ! -x $appimage ]]; then
    printf 'AppImage must exist and be executable: %s\n' "$appimage" >&2
    exit 2
fi
if [[ ! -r /dev/fuse || ! -w /dev/fuse ]]; then
    printf '/dev/fuse is unavailable to the current user\n' >&2
    exit 77
fi
if ! command -v fusermount3 >/dev/null 2>&1; then
    printf 'fusermount3 is required\n' >&2
    exit 77
fi
if ! grep -a -q 'libfuse.so.2' "$appimage"; then
    printf 'the AppImage does not appear to contain a legacy FUSE2 runtime\n' >&2
    exit 2
fi

project_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
make -C "$project_root" all

log=$(mktemp)
mounts_before=$(mktemp)
mounts_after=$(mktemp)
cleanup() {
    rm -f "$log" "$mounts_before" "$mounts_after"
}
trap cleanup EXIT

findmnt -rn -t fuse,fuse3,fuseblk -o TARGET,SOURCE >"$mounts_before" || true

if (( $# == 0 )); then
    set -- --help
fi

set +e
LD_LIBRARY_PATH="$project_root/build${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" \
LIBFUSE2_COMPAT_LOG=1 \
timeout --signal=TERM 30 "$appimage" "$@" 2> >(tee "$log" >&2)
status=$?
set -e

if (( status != 0 )); then
    printf 'legacy AppImage exited with status %d\n' "$status" >&2
    exit 1
fi
if ! grep -q 'libfuse2-compat: using backend libfuse3.so' "$log"; then
    printf 'the AppImage did not load libfuse2-compat\n' >&2
    exit 1
fi

findmnt -rn -t fuse,fuse3,fuseblk -o TARGET,SOURCE >"$mounts_after" || true
if ! diff -u "$mounts_before" "$mounts_after"; then
    printf 'a FUSE mount remained after the AppImage exited\n' >&2
    exit 1
fi

printf 'legacy AppImage integration test: ok\n'
