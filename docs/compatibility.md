# Compatibility

## Expected to work

- Linux x86_64
- legacy AppImageKit type-2 runtimes that dynamically load `libfuse.so.2`
- FUSE3 libraries with SONAME `libfuse3.so.3` or `libfuse3.so.4`
- normal unprivileged FUSE mounts through `fusermount3`

## Does not need the bridge

- current AppImage `type2-runtime` builds that already use FUSE3
- AppImages using extract-and-run instead of FUSE

## Not implemented yet

- FUSE2 high-level `fuse_main_real`
- CUSE
- the complete generic libfuse2 API
- BSD and macOS FUSE implementations
- guaranteed 32-bit support

## Identifying a legacy AppImage

```bash
grep -a -q 'libfuse.so.2' ./Application.AppImage && echo legacy-runtime
```

This string check is intentionally simple. The integration harness performs it
before attempting a test.
