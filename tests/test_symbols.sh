#!/usr/bin/env bash
set -euo pipefail

library=${1:?usage: test_symbols.sh LIBRARY}

soname=$(readelf -d "$library" | sed -n 's/.*SONAME.*\[\(.*\)\].*/\1/p')
if [[ $soname != libfuse.so.2 ]]; then
    printf 'unexpected SONAME: %s\n' "$soname" >&2
    exit 1
fi

expected=(
    fuse_add_direntry fuse_daemonize fuse_lowlevel_new fuse_mount
    fuse_opt_add_arg fuse_opt_free_args fuse_opt_parse fuse_parse_cmdline
    fuse_remove_signal_handlers fuse_reply_attr fuse_reply_buf
    fuse_reply_bmap fuse_reply_create fuse_reply_entry fuse_reply_err
    fuse_reply_lock fuse_reply_none fuse_reply_open fuse_reply_readlink
    fuse_reply_statfs fuse_reply_write
    fuse_reply_xattr fuse_req_userdata fuse_session_add_chan
    fuse_session_destroy fuse_session_exit fuse_session_exited
    fuse_session_loop fuse_session_remove_chan fuse_session_reset
    fuse_set_signal_handlers fuse_unmount fuse_version
)

symbol_table=$(objdump -T "$library")
symbols=$(awk '$4 == ".text" { print $NF }' <<<"$symbol_table" | sort -u)
for symbol in "${expected[@]}"; do
    if ! grep -qx "$symbol" <<<"$symbols"; then
        printf 'missing exported symbol: %s\n' "$symbol" >&2
        exit 1
    fi
done

if ! grep -q 'FUSE_2.6.*fuse_lowlevel_new' <<<"$symbol_table"; then
    printf 'fuse_lowlevel_new has the wrong symbol version\n' >&2
    exit 1
fi
if ! grep -q 'FUSE_2.5.*fuse_opt_parse' <<<"$symbol_table"; then
    printf 'fuse_opt_parse has the wrong symbol version\n' >&2
    exit 1
fi
if ! grep -q 'FUSE_2.4.*fuse_reply_err' <<<"$symbol_table"; then
    printf 'fuse_reply_err has the wrong symbol version\n' >&2
    exit 1
fi
if ! grep -q 'FUSE_2.7.5.*fuse_reply_bmap' <<<"$symbol_table"; then
    printf 'fuse_reply_bmap has the wrong symbol version\n' >&2
    exit 1
fi

printf 'symbol ABI test: ok\n'
