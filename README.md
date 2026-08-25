# libfuse2-compat

`libfuse2-compat` lets legacy type-2 AppImages use a FUSE3-only userspace.
It installs a real `libfuse.so.2` compatibility library and translates the
FUSE2 ABI used by old AppImage runtimes into the FUSE3 API.

The bridge is transparent after installation. Existing AppImages keep calling
`dlopen("libfuse.so.2")`; no wrapper, repackaging, `LD_PRELOAD`, or custom
kernel module is required.

> [!IMPORTANT]
> Linux still needs its normal FUSE kernel support, `/dev/fuse`, and
> `fusermount3`. This project removes the obsolete **FUSE2 userspace library**,
> not the kernel side of FUSE.

## Status

The current implementation targets the low-level ABI used by the legacy
AppImageKit type-2 runtime and its embedded SquashFUSE reader.

Implemented and tested:

- `libfuse.so.2` SONAME and FUSE2 symbol versions
- FUSE2 channel lifecycle translated to FUSE3 sessions
- FUSE2/FUSE3 `fuse_file_info` conversion
- FUSE2/FUSE3 `fuse_conn_info` conversion
- FUSE2/FUSE3 `fuse_entry_param` conversion
- argument parsing and option forwarding
- lookup, attributes, file reads, directory reads, xattrs, statfs and forget
- FUSE3 backend discovery for `libfuse3.so.3` and `libfuse3.so.4`
- unit tests without `/dev/fuse`
- real legacy AppImage integration harness

The current AppImage
[type2-runtime](https://github.com/AppImage/type2-runtime) already builds
against FUSE3. This bridge exists for AppImages carrying older runtimes that
still load `libfuse.so.2`.

## Build

Requirements:

- a C11 compiler
- GNU make
- `libdl` and pthreads
- FUSE3 installed at runtime

No FUSE2 development package is used or required.

```bash
make
make check
```

## Test without installing

```bash
LD_LIBRARY_PATH="$PWD/build" \
LIBFUSE2_COMPAT_LOG=1 \
./Legacy.AppImage --help
```

For the complete integration test:

```bash
make check-appimage APPIMAGE="$PWD/Legacy.AppImage"
```

The AppImage must contain a legacy runtime that references `libfuse.so.2`.

## Install

```bash
sudo make install
sudo ldconfig
```

The default destination is `/usr/local/lib`. Override it with `PREFIX` or
`LIBDIR`:

```bash
sudo make install PREFIX=/usr LIBDIR=/usr/lib/x86_64-linux-gnu
sudo ldconfig
```

After installation, launch legacy AppImages normally.

## Diagnostics

```bash
LIBFUSE2_COMPAT_LOG=1 ./Legacy.AppImage
```

To select an exact FUSE3 backend during testing:

```bash
LIBFUSE2_COMPAT_BACKEND=/path/to/libfuse3.so.3 ./Legacy.AppImage
```

## Scope

This is deliberately an AppImage compatibility project, not a promise of
complete source or binary compatibility for every filesystem ever built with
libfuse2. The legacy high-level `fuse_main_real` API and CUSE are not yet
implemented.

See [Architecture](docs/architecture.md), [Testing](docs/testing.md), and
[Compatibility](docs/compatibility.md).

## License

LGPL-2.1-or-later, matching libfuse's compatibility requirements.
