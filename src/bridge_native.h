/* SPDX-License-Identifier: LGPL-2.1-or-later */
#ifndef LIBFUSE2_COMPAT_BRIDGE_NATIVE_H
#define LIBFUSE2_COMPAT_BRIDGE_NATIVE_H

#include "fuse2_abi.h"

typedef void (*fuse2_native_init_proc_t)(void *userdata,
					 struct fuse2_conn_info *conn);
typedef void (*fuse2_native_destroy_proc_t)(void *userdata);
typedef void (*fuse2_native_lookup_proc_t)(fuse2_req_t req, fuse2_ino_t parent,
					   const char *name);
typedef void (*fuse2_native_forget_proc_t)(fuse2_req_t req, fuse2_ino_t ino,
					   unsigned long nlookup);
typedef void (*fuse2_native_file_info_proc_t)(
	fuse2_req_t req, fuse2_ino_t ino, struct fuse2_file_info *fi);
typedef void (*fuse2_native_setattr_proc_t)(
	fuse2_req_t req, fuse2_ino_t ino, struct stat *attr, int to_set,
	struct fuse2_file_info *fi);
typedef void (*fuse2_native_inode_proc_t)(fuse2_req_t req, fuse2_ino_t ino);
typedef void (*fuse2_native_read_proc_t)(
	fuse2_req_t req, fuse2_ino_t ino, size_t size, off_t off,
	struct fuse2_file_info *fi);
typedef void (*fuse2_native_write_proc_t)(
	fuse2_req_t req, fuse2_ino_t ino, const char *buf, size_t size, off_t off,
	struct fuse2_file_info *fi);
typedef void (*fuse2_native_getxattr_proc_t)(
	fuse2_req_t req, fuse2_ino_t ino, const char *name, size_t size);
typedef void (*fuse2_native_listxattr_proc_t)(fuse2_req_t req, fuse2_ino_t ino,
					      size_t size);
typedef void (*fuse2_native_create_proc_t)(
	fuse2_req_t req, fuse2_ino_t parent, const char *name, mode_t mode,
	struct fuse2_file_info *fi);

void fuse2_foundation_bridge_init(fuse2_native_init_proc_t callback,
				   void *userdata,
				   struct fuse2_conn_info *conn);
void fuse2_foundation_bridge_destroy(fuse2_native_destroy_proc_t callback,
				      void *userdata);
void fuse2_foundation_bridge_lookup(fuse2_native_lookup_proc_t callback,
				     fuse2_req_t req, fuse2_ino_t parent,
				     const char *name);
void fuse2_foundation_bridge_forget(fuse2_native_forget_proc_t callback,
				     fuse2_req_t req, fuse2_ino_t ino,
				     unsigned long nlookup);
void fuse2_foundation_bridge_getattr(fuse2_native_file_info_proc_t callback,
				      fuse2_req_t req, fuse2_ino_t ino,
				      struct fuse2_file_info *fi);
void fuse2_foundation_bridge_setattr(fuse2_native_setattr_proc_t callback,
				      fuse2_req_t req, fuse2_ino_t ino,
				      struct stat *attr, int to_set,
				      struct fuse2_file_info *fi);
void fuse2_foundation_bridge_readlink(fuse2_native_inode_proc_t callback,
				      fuse2_req_t req, fuse2_ino_t ino);
void fuse2_foundation_bridge_open(fuse2_native_file_info_proc_t callback,
				   fuse2_req_t req, fuse2_ino_t ino,
				   struct fuse2_file_info *fi);
void fuse2_foundation_bridge_read(fuse2_native_read_proc_t callback,
				   fuse2_req_t req, fuse2_ino_t ino, size_t size,
				   off_t off, struct fuse2_file_info *fi);
void fuse2_foundation_bridge_write(fuse2_native_write_proc_t callback,
				    fuse2_req_t req, fuse2_ino_t ino,
				    const char *buf, size_t size, off_t off,
				    struct fuse2_file_info *fi);
void fuse2_foundation_bridge_release(fuse2_native_file_info_proc_t callback,
				      fuse2_req_t req, fuse2_ino_t ino,
				      struct fuse2_file_info *fi);
void fuse2_foundation_bridge_opendir(fuse2_native_file_info_proc_t callback,
				      fuse2_req_t req, fuse2_ino_t ino,
				      struct fuse2_file_info *fi);
void fuse2_foundation_bridge_readdir(fuse2_native_read_proc_t callback,
				      fuse2_req_t req, fuse2_ino_t ino,
				      size_t size, off_t off,
				      struct fuse2_file_info *fi);
void fuse2_foundation_bridge_releasedir(fuse2_native_file_info_proc_t callback,
					 fuse2_req_t req, fuse2_ino_t ino,
					 struct fuse2_file_info *fi);
void fuse2_foundation_bridge_statfs(fuse2_native_inode_proc_t callback,
				     fuse2_req_t req, fuse2_ino_t ino);
void fuse2_foundation_bridge_getxattr(fuse2_native_getxattr_proc_t callback,
				      fuse2_req_t req, fuse2_ino_t ino,
				      const char *name, size_t size);
void fuse2_foundation_bridge_listxattr(fuse2_native_listxattr_proc_t callback,
				       fuse2_req_t req, fuse2_ino_t ino,
				       size_t size);
void fuse2_foundation_bridge_create(fuse2_native_create_proc_t callback,
				     fuse2_req_t req, fuse2_ino_t parent,
				     const char *name, mode_t mode,
				     struct fuse2_file_info *fi);

int fuse2_native_opt_parse(struct fuse2_args *args, void *data,
			   const struct fuse2_opt opts[], fuse2_opt_proc_t proc);
int fuse2_native_opt_add_arg(struct fuse2_args *args, const char *arg);
void fuse2_native_opt_free_args(struct fuse2_args *args);
int fuse2_native_parse_cmdline(struct fuse2_args *args, char **mountpoint,
			       int *multithreaded, int *foreground);
int fuse2_native_daemonize(int foreground);

struct fuse2_chan *fuse2_native_mount(const char *mountpoint,
				      struct fuse2_args *args);
struct fuse2_session *fuse2_native_lowlevel_new(
	struct fuse2_args *args, const struct fuse2_lowlevel_ops *ops,
	size_t op_size, void *userdata);
void fuse2_native_session_add_chan(struct fuse2_session *session,
				   struct fuse2_chan *chan);
void fuse2_native_session_remove_chan(struct fuse2_chan *chan);
int fuse2_native_session_loop(struct fuse2_session *session);
void fuse2_native_session_destroy(struct fuse2_session *session);
void fuse2_native_unmount(const char *mountpoint, struct fuse2_chan *chan);
int fuse2_native_set_signal_handlers(struct fuse2_session *session);
void fuse2_native_remove_signal_handlers(struct fuse2_session *session);
void fuse2_native_session_exit(struct fuse2_session *session);
int fuse2_native_session_exited(struct fuse2_session *session);
void fuse2_native_session_reset(struct fuse2_session *session);

void *fuse2_native_req_userdata(fuse2_req_t req);
int fuse2_native_reply_err(fuse2_req_t req, int err);
void fuse2_native_reply_none(fuse2_req_t req);
int fuse2_native_reply_entry(fuse2_req_t req,
			     const struct fuse2_entry_param *entry);
int fuse2_native_reply_attr(fuse2_req_t req, const struct stat *attr,
			    double attr_timeout);
int fuse2_native_reply_open(fuse2_req_t req,
			    const struct fuse2_file_info *fi);
int fuse2_native_reply_buf(fuse2_req_t req, const char *buf, size_t size);
int fuse2_native_reply_readlink(fuse2_req_t req, const char *link);
int fuse2_native_reply_xattr(fuse2_req_t req, size_t count);
int fuse2_native_reply_write(fuse2_req_t req, size_t count);
int fuse2_native_reply_statfs(fuse2_req_t req,
			      const struct statvfs *statfsbuf);
int fuse2_native_reply_create(fuse2_req_t req,
			      const struct fuse2_entry_param *entry,
			      const struct fuse2_file_info *fi);
int fuse2_native_reply_lock(fuse2_req_t req, const struct flock *lock);
int fuse2_native_reply_bmap(fuse2_req_t req, uint64_t idx);
size_t fuse2_native_add_direntry(fuse2_req_t req, char *buf, size_t bufsize,
				 const char *name,
				 const struct stat *statbuf, off_t off);

#endif
