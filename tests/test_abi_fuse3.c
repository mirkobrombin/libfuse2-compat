/* SPDX-License-Identifier: LGPL-2.1-or-later */
#define FUSE_USE_VERSION 30
#ifdef LIBFUSE2_COMPAT_FLAT_FUSE3_HEADERS
#include <fuse_lowlevel.h>
#else
#include <fuse3/fuse_lowlevel.h>
#endif

#include "fuse3_abi.h"

#include <stddef.h>

#define SAME_OFFSET(native_type, compat_type, member) \
	_Static_assert(offsetof(native_type, member) == offsetof(compat_type, member), \
		       "offset mismatch: " #member)

_Static_assert(sizeof(struct fuse_args) == sizeof(struct fuse3_args),
	       "fuse_args size mismatch");
_Static_assert(sizeof(struct fuse_opt) == sizeof(struct fuse3_opt),
	       "fuse_opt size mismatch");

SAME_OFFSET(struct fuse_file_info, struct fuse3_file_info, flags);
SAME_OFFSET(struct fuse_file_info, struct fuse3_file_info, fh);
SAME_OFFSET(struct fuse_file_info, struct fuse3_file_info, lock_owner);
SAME_OFFSET(struct fuse_file_info, struct fuse3_file_info, poll_events);
SAME_OFFSET(struct fuse_conn_info, struct fuse3_conn_info, max_write);
SAME_OFFSET(struct fuse_conn_info, struct fuse3_conn_info, max_readahead);
SAME_OFFSET(struct fuse_conn_info, struct fuse3_conn_info, capable);
SAME_OFFSET(struct fuse_lowlevel_ops, struct fuse3_lowlevel_ops, lookup);
SAME_OFFSET(struct fuse_lowlevel_ops, struct fuse3_lowlevel_ops, getattr);
SAME_OFFSET(struct fuse_lowlevel_ops, struct fuse3_lowlevel_ops, rename);
SAME_OFFSET(struct fuse_lowlevel_ops, struct fuse3_lowlevel_ops, open);
SAME_OFFSET(struct fuse_lowlevel_ops, struct fuse3_lowlevel_ops, readdir);
SAME_OFFSET(struct fuse_lowlevel_ops, struct fuse3_lowlevel_ops, create);
SAME_OFFSET(struct fuse_lowlevel_ops, struct fuse3_lowlevel_ops, fallocate);
_Static_assert(offsetof(struct fuse_lowlevel_ops, readdirplus) ==
	       sizeof(struct fuse3_lowlevel_ops),
	       "FUSE3 operation prefix size mismatch");

int main(void)
{
	return 0;
}
