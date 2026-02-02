# libfuse2-compat

A userspace `libfuse.so.2` compatibility library for legacy type-2 AppImages
on FUSE3-only systems.

Old AppImageKit runtimes still `dlopen("libfuse.so.2")`. This project ships
that SONAME and forwards the low-level FUSE2 ABI they rely on to a dynamically
loaded FUSE3 backend, without repackaging the AppImage, `LD_PRELOAD`, or a
custom kernel module.

## Scope

The target is the low-level ABI used by the legacy AppImageKit type-2 runtime
and its embedded SquashFUSE reader. The high-level `fuse_main_real` API and CUSE
are out of scope.

## License

LGPL-2.1-or-later, matching libfuse.
