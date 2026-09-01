# Testing

## Tests available everywhere

`make check` runs:

- a fake FUSE3 backend that exercises the complete AppImage lifecycle
- callback and reply conversion checks
- ELF SONAME and symbol-version checks
- Foundation bridge presence and hidden native adapter symbols
- a loader probe against the system FUSE3 library
- header ABI checks when FUSE2/FUSE3 development headers are available

These tests do not mount anything and therefore work in restricted containers.

## Real AppImage test

The real integration test requires:

- readable and writable `/dev/fuse`
- `fusermount3`
- a legacy AppImage containing `libfuse.so.2`
- no installed FUSE2 library ahead of the build directory

Run:

```bash
make check-appimage APPIMAGE=/absolute/path/Legacy.AppImage
```

A pinned AppImageKit 13 fixture can be downloaded with:

```bash
./tests/fetch_legacy_fixture.sh
./tests/integration_appimage.sh \
  ./tests/fixtures/obsolete-appimagetool-x86_64.AppImage --help
```

The harness checks that the compatibility bridge loaded FUSE3, the AppImage
returned successfully, and no mount owned by the AppImage remained behind.

If the payload does not support `--help`, pass another short command that exits
without user interaction.

## Debug a failure

```bash
LIBFUSE2_COMPAT_LOG=1 \
LD_LIBRARY_PATH="$PWD/build" \
strace -f -e openat,mount,umount2,execve \
./Legacy.AppImage --help
```

Attach the command output, AppImage name, architecture, distribution,
`libfuse3` package version, and `fusermount3 --version` to the issue.
