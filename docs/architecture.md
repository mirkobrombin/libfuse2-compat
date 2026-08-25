# Architecture

## Why a symlink cannot work

FUSE2 and FUSE3 expose similarly named functions, but they are not ABI
compatible. In particular:

- FUSE2 mounts first and attaches a channel to a session later.
- FUSE3 creates a session first and mounts that session directly.
- `fuse_file_info` fields have different positions.
- `fuse_conn_info` changed the position and meaning of several fields.
- the low-level `rename` callback gained a flags argument.

Pointing `libfuse.so.2` at `libfuse3.so` can therefore corrupt callback data.

## Translation model

`libfuse2-compat` exposes the ABI and symbol versions of `libfuse.so.2`, but
loads FUSE3 dynamically.

1. `fuse_mount()` creates a compatibility channel and remembers the
   mountpoint. It intentionally delays the real mount.
2. `fuse_lowlevel_new()` creates a FUSE3 session with bridge callbacks.
3. `fuse_session_add_chan()` mounts the FUSE3 session at the remembered
   mountpoint.
4. Requests arrive through FUSE3 callbacks. The bridge converts ABI-sensitive
   structures and calls the original FUSE2 callback.
5. FUSE2 reply functions convert data back and forward the reply to FUSE3.
6. Channel removal and destruction are reordered into the lifecycle expected
   by FUSE3.

Each session owns its callback table and user data. Request routing uses the
userdata stored in the corresponding FUSE3 request, so concurrent AppImages do
not share mutable session state.

## Backend loading

The loader tries:

1. `LIBFUSE2_COMPAT_BACKEND`, when explicitly set
2. `libfuse3.so.4`
3. `libfuse3.so.3`

The bridge uses the stable FUSE 3.0 low-level operation prefix. Newer
operations are excluded through the `op_size` passed to FUSE3.

## Targeted AppImage ABI

The legacy AppImageKit runtime uses SquashFUSE's low-level operations:

- lookup and forget
- getattr and readlink
- open, read and release
- opendir, readdir and releasedir
- getxattr and listxattr
- statfs
- read-only create rejection

All required reply and lifecycle functions are exported with their historical
FUSE2 symbol versions.
