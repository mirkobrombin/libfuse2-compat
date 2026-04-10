#!/usr/bin/env bash
set -euo pipefail

cc=${CC:-cc}
cflags=(-std=c11 -Wall -Wextra -Werror -D_FILE_OFFSET_BITS=64 -Isrc)
ran=0

if ! command -v pkg-config >/dev/null 2>&1; then
    printf 'header ABI tests: skipped, pkg-config unavailable\n'
    exit 0
fi

if pkg-config --exists fuse; then
    # shellcheck disable=SC2207
    fuse2_flags=($(pkg-config --cflags fuse))
    "$cc" "${cflags[@]}" "${fuse2_flags[@]}" tests/test_abi_fuse2.c \
        -o tests/build/test_abi_fuse2
    tests/build/test_abi_fuse2
    printf 'FUSE2 header ABI test: ok\n'
    ran=1
fi

if pkg-config --exists fuse3; then
    # shellcheck disable=SC2207
    fuse3_flags=($(pkg-config --cflags fuse3))
    "$cc" "${cflags[@]}" "${fuse3_flags[@]}" tests/test_abi_fuse3.c \
        -o tests/build/test_abi_fuse3
    tests/build/test_abi_fuse3
    printf 'FUSE3 header ABI test: ok\n'
    ran=1
fi

if (( ran == 0 )); then
    printf 'header ABI tests: skipped, development headers unavailable\n'
fi
