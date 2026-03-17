/* SPDX-License-Identifier: LGPL-2.1-or-later */
#ifndef LIBFUSE2_COMPAT_FUSE3_LOADER_H
#define LIBFUSE2_COMPAT_FUSE3_LOADER_H

#include "fuse3_abi.h"

struct fuse3_api {
	void *handle;
	const char *backend_name;

	int (*opt_parse)(struct fuse3_args *args, void *data,
			 const struct fuse3_opt opts[], fuse3_opt_proc_t proc);
	int (*opt_add_arg)(struct fuse3_args *args, const char *arg);
	void (*opt_free_args)(struct fuse3_args *args);
	int (*parse_cmdline)(struct fuse3_args *args,
			     struct fuse3_cmdline_opts *opts);

	struct fuse3_session *(*session_new)(struct fuse3_args *args,
					    const struct fuse3_lowlevel_ops *ops,
					    size_t op_size, void *userdata);
	int (*session_mount)(struct fuse3_session *session,
			     const char *mountpoint);
	void (*session_unmount)(struct fuse3_session *session);
	int (*session_loop)(struct fuse3_session *session);
	void (*session_destroy)(struct fuse3_session *session);
	void (*session_exit)(struct fuse3_session *session);
	int (*session_exited)(struct fuse3_session *session);
	void (*session_reset)(struct fuse3_session *session);
	int (*set_signal_handlers)(struct fuse3_session *session);
	void (*remove_signal_handlers)(struct fuse3_session *session);
	int (*daemonize)(int foreground);

	void *(*req_userdata)(fuse3_req_t req);
	int (*reply_err)(fuse3_req_t req, int err);
	void (*reply_none)(fuse3_req_t req);
	int (*reply_entry)(fuse3_req_t req, const struct fuse3_entry_param *entry);
	int (*reply_attr)(fuse3_req_t req, const struct stat *attr,
			  double attr_timeout);
	int (*reply_open)(fuse3_req_t req, const struct fuse3_file_info *fi);
	int (*reply_buf)(fuse3_req_t req, const char *buf, size_t size);
	int (*reply_readlink)(fuse3_req_t req, const char *link);
	int (*reply_xattr)(fuse3_req_t req, size_t count);
	int (*reply_write)(fuse3_req_t req, size_t count);
	int (*reply_statfs)(fuse3_req_t req, const struct statvfs *statfsbuf);
	int (*reply_create)(fuse3_req_t req,
			    const struct fuse3_entry_param *entry,
			    const struct fuse3_file_info *fi);
	int (*reply_lock)(fuse3_req_t req, const struct flock *lock);
	int (*reply_bmap)(fuse3_req_t req, uint64_t idx);
	size_t (*add_direntry)(fuse3_req_t req, char *buf, size_t bufsize,
			       const char *name, const struct stat *statbuf,
			       off_t off);
};

const struct fuse3_api *fuse3_api_get(void);
const char *fuse3_api_error(void);
void fuse2_compat_log(const char *format, ...)
	__attribute__((format(printf, 1, 2)));

#endif
