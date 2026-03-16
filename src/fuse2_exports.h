/* SPDX-License-Identifier: LGPL-2.1-or-later */
#ifndef LIBFUSE2_COMPAT_EXPORTS_H
#define LIBFUSE2_COMPAT_EXPORTS_H

#include "fuse2_abi.h"

#define FUSE2_EXPORT __attribute__((visibility("default")))

FUSE2_EXPORT int fuse_opt_parse(struct fuse2_args *args, void *data,
			       const struct fuse2_opt opts[],
			       fuse2_opt_proc_t proc);
FUSE2_EXPORT int fuse_opt_add_arg(struct fuse2_args *args, const char *arg);
FUSE2_EXPORT void fuse_opt_free_args(struct fuse2_args *args);
FUSE2_EXPORT int fuse_parse_cmdline(struct fuse2_args *args, char **mountpoint,
				   int *multithreaded, int *foreground);
FUSE2_EXPORT int fuse_daemonize(int foreground);
FUSE2_EXPORT int fuse_version(void);

FUSE2_EXPORT struct fuse2_chan *fuse_mount(const char *mountpoint,
					   struct fuse2_args *args);
FUSE2_EXPORT void fuse_unmount(const char *mountpoint, struct fuse2_chan *chan);
FUSE2_EXPORT struct fuse2_session *fuse_lowlevel_new(
	struct fuse2_args *args, const struct fuse2_lowlevel_ops *ops,
	size_t op_size, void *userdata);
FUSE2_EXPORT void fuse_session_add_chan(struct fuse2_session *session,
					struct fuse2_chan *chan);
FUSE2_EXPORT void fuse_session_remove_chan(struct fuse2_chan *chan);
FUSE2_EXPORT int fuse_session_loop(struct fuse2_session *session);
FUSE2_EXPORT void fuse_session_destroy(struct fuse2_session *session);
FUSE2_EXPORT void fuse_session_exit(struct fuse2_session *session);
FUSE2_EXPORT int fuse_session_exited(struct fuse2_session *session);
FUSE2_EXPORT void fuse_session_reset(struct fuse2_session *session);
FUSE2_EXPORT int fuse_set_signal_handlers(struct fuse2_session *session);
FUSE2_EXPORT void fuse_remove_signal_handlers(struct fuse2_session *session);

FUSE2_EXPORT void *fuse_req_userdata(fuse2_req_t req);
FUSE2_EXPORT int fuse_reply_err(fuse2_req_t req, int err);
FUSE2_EXPORT void fuse_reply_none(fuse2_req_t req);
FUSE2_EXPORT int fuse_reply_entry(fuse2_req_t req,
				  const struct fuse2_entry_param *entry);
FUSE2_EXPORT int fuse_reply_attr(fuse2_req_t req, const struct stat *attr,
				double attr_timeout);
FUSE2_EXPORT int fuse_reply_open(fuse2_req_t req,
				 const struct fuse2_file_info *fi);
FUSE2_EXPORT int fuse_reply_buf(fuse2_req_t req, const char *buf, size_t size);
FUSE2_EXPORT int fuse_reply_readlink(fuse2_req_t req, const char *link);
FUSE2_EXPORT int fuse_reply_xattr(fuse2_req_t req, size_t count);
FUSE2_EXPORT int fuse_reply_write(fuse2_req_t req, size_t count);
FUSE2_EXPORT int fuse_reply_statfs(fuse2_req_t req,
				   const struct statvfs *statfsbuf);
FUSE2_EXPORT int fuse_reply_create(fuse2_req_t req,
				   const struct fuse2_entry_param *entry,
				   const struct fuse2_file_info *fi);
FUSE2_EXPORT int fuse_reply_lock(fuse2_req_t req, const struct flock *lock);
FUSE2_EXPORT int fuse_reply_bmap(fuse2_req_t req, uint64_t idx);
FUSE2_EXPORT size_t fuse_add_direntry(fuse2_req_t req, char *buf,
				     size_t bufsize, const char *name,
				     const struct stat *statbuf, off_t off);

#endif
