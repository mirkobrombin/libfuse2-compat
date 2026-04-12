/* SPDX-License-Identifier: LGPL-2.1-or-later */
#define FUSE_USE_VERSION 26
#include <fuse_lowlevel.h>

#include "fuse2_abi.h"

#include <stddef.h>

#define SAME_SIZE(native_type, compat_type) \
	_Static_assert(sizeof(native_type) == sizeof(compat_type), "size mismatch")
#define SAME_OFFSET(native_type, compat_type, member) \
	_Static_assert(offsetof(native_type, member) == offsetof(compat_type, member), \
		       "offset mismatch: " #member)

SAME_SIZE(struct fuse_args, struct fuse2_args);
SAME_SIZE(struct fuse_opt, struct fuse2_opt);
SAME_SIZE(struct fuse_file_info, struct fuse2_file_info);
SAME_SIZE(struct fuse_conn_info, struct fuse2_conn_info);
SAME_SIZE(struct fuse_entry_param, struct fuse2_entry_param);
SAME_SIZE(struct fuse_lowlevel_ops, struct fuse2_lowlevel_ops);

SAME_OFFSET(struct fuse_file_info, struct fuse2_file_info, flags);
SAME_OFFSET(struct fuse_file_info, struct fuse2_file_info, fh_old);
SAME_OFFSET(struct fuse_file_info, struct fuse2_file_info, writepage);
SAME_OFFSET(struct fuse_file_info, struct fuse2_file_info, fh);
SAME_OFFSET(struct fuse_file_info, struct fuse2_file_info, lock_owner);
SAME_OFFSET(struct fuse_conn_info, struct fuse2_conn_info, max_write);
SAME_OFFSET(struct fuse_conn_info, struct fuse2_conn_info, capable);
SAME_OFFSET(struct fuse_lowlevel_ops, struct fuse2_lowlevel_ops, lookup);
SAME_OFFSET(struct fuse_lowlevel_ops, struct fuse2_lowlevel_ops, getattr);
SAME_OFFSET(struct fuse_lowlevel_ops, struct fuse2_lowlevel_ops, open);
SAME_OFFSET(struct fuse_lowlevel_ops, struct fuse2_lowlevel_ops, readdir);
SAME_OFFSET(struct fuse_lowlevel_ops, struct fuse2_lowlevel_ops, create);
SAME_OFFSET(struct fuse_lowlevel_ops, struct fuse2_lowlevel_ops, fallocate);

int main(void)
{
	return 0;
}
