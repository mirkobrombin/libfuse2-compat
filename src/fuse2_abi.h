/* SPDX-License-Identifier: LGPL-2.1-or-later */
#ifndef LIBFUSE2_COMPAT_FUSE2_ABI_H
#define LIBFUSE2_COMPAT_FUSE2_ABI_H

#include <fcntl.h>
#include <stdint.h>
#include <stddef.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/types.h>

typedef unsigned long fuse2_ino_t;
typedef struct fuse2_req *fuse2_req_t;

struct fuse2_session;
struct fuse2_chan;
struct fuse2_pollhandle;

struct fuse2_args {
	int argc;
	char **argv;
	int allocated;
};

struct fuse2_opt {
	const char *templ;
	unsigned long offset;
	int value;
};

typedef int (*fuse2_opt_proc_t)(void *data, const char *arg, int key,
				struct fuse2_args *outargs);

struct fuse2_file_info {
	int flags;
	unsigned long fh_old;
	int writepage;
	unsigned int direct_io : 1;
	unsigned int keep_cache : 1;
	unsigned int flush : 1;
	unsigned int nonseekable : 1;
	unsigned int flock_release : 1;
	unsigned int padding : 27;
	uint64_t fh;
	uint64_t lock_owner;
};

struct fuse2_conn_info {
	unsigned int proto_major;
	unsigned int proto_minor;
	unsigned int async_read;
	unsigned int max_write;
	unsigned int max_readahead;
	unsigned int capable;
	unsigned int want;
	unsigned int max_background;
	unsigned int congestion_threshold;
	unsigned int reserved[23];
};

struct fuse2_entry_param {
	fuse2_ino_t ino;
	unsigned long generation;
	struct stat attr;
	double attr_timeout;
	double entry_timeout;
};

enum fuse2_buf_flags {
	FUSE2_BUF_IS_FD = 1 << 1,
	FUSE2_BUF_FD_SEEK = 1 << 2,
	FUSE2_BUF_FD_RETRY = 1 << 3,
};

struct fuse2_buf {
	size_t size;
	enum fuse2_buf_flags flags;
	void *mem;
	int fd;
	off_t pos;
};

struct fuse2_bufvec {
	size_t count;
	size_t idx;
	size_t off;
	struct fuse2_buf buf[1];
};

struct fuse2_forget_data {
	uint64_t ino;
	uint64_t nlookup;
};

struct fuse2_lowlevel_ops {
	void (*init)(void *userdata, struct fuse2_conn_info *conn);
	void (*destroy)(void *userdata);
	void (*lookup)(fuse2_req_t req, fuse2_ino_t parent, const char *name);
	void (*forget)(fuse2_req_t req, fuse2_ino_t ino, unsigned long nlookup);
	void (*getattr)(fuse2_req_t req, fuse2_ino_t ino, struct fuse2_file_info *fi);
	void (*setattr)(fuse2_req_t req, fuse2_ino_t ino, struct stat *attr,
		       int to_set, struct fuse2_file_info *fi);
	void (*readlink)(fuse2_req_t req, fuse2_ino_t ino);
	void (*mknod)(fuse2_req_t req, fuse2_ino_t parent, const char *name,
		      mode_t mode, dev_t rdev);
	void (*mkdir)(fuse2_req_t req, fuse2_ino_t parent, const char *name,
		      mode_t mode);
	void (*unlink)(fuse2_req_t req, fuse2_ino_t parent, const char *name);
	void (*rmdir)(fuse2_req_t req, fuse2_ino_t parent, const char *name);
	void (*symlink)(fuse2_req_t req, const char *link, fuse2_ino_t parent,
			const char *name);
	void (*rename)(fuse2_req_t req, fuse2_ino_t parent, const char *name,
		       fuse2_ino_t newparent, const char *newname);
	void (*link)(fuse2_req_t req, fuse2_ino_t ino, fuse2_ino_t newparent,
		     const char *newname);
	void (*open)(fuse2_req_t req, fuse2_ino_t ino, struct fuse2_file_info *fi);
	void (*read)(fuse2_req_t req, fuse2_ino_t ino, size_t size, off_t off,
		     struct fuse2_file_info *fi);
	void (*write)(fuse2_req_t req, fuse2_ino_t ino, const char *buf,
		      size_t size, off_t off, struct fuse2_file_info *fi);
	void (*flush)(fuse2_req_t req, fuse2_ino_t ino, struct fuse2_file_info *fi);
	void (*release)(fuse2_req_t req, fuse2_ino_t ino, struct fuse2_file_info *fi);
	void (*fsync)(fuse2_req_t req, fuse2_ino_t ino, int datasync,
		      struct fuse2_file_info *fi);
	void (*opendir)(fuse2_req_t req, fuse2_ino_t ino, struct fuse2_file_info *fi);
	void (*readdir)(fuse2_req_t req, fuse2_ino_t ino, size_t size, off_t off,
			struct fuse2_file_info *fi);
	void (*releasedir)(fuse2_req_t req, fuse2_ino_t ino,
			   struct fuse2_file_info *fi);
	void (*fsyncdir)(fuse2_req_t req, fuse2_ino_t ino, int datasync,
			 struct fuse2_file_info *fi);
	void (*statfs)(fuse2_req_t req, fuse2_ino_t ino);
	void (*setxattr)(fuse2_req_t req, fuse2_ino_t ino, const char *name,
			 const char *value, size_t size, int flags);
	void (*getxattr)(fuse2_req_t req, fuse2_ino_t ino, const char *name,
			 size_t size);
	void (*listxattr)(fuse2_req_t req, fuse2_ino_t ino, size_t size);
	void (*removexattr)(fuse2_req_t req, fuse2_ino_t ino, const char *name);
	void (*access)(fuse2_req_t req, fuse2_ino_t ino, int mask);
	void (*create)(fuse2_req_t req, fuse2_ino_t parent, const char *name,
		       mode_t mode, struct fuse2_file_info *fi);
	void (*getlk)(fuse2_req_t req, fuse2_ino_t ino, struct fuse2_file_info *fi,
		      struct flock *lock);
	void (*setlk)(fuse2_req_t req, fuse2_ino_t ino, struct fuse2_file_info *fi,
		      struct flock *lock, int sleep);
	void (*bmap)(fuse2_req_t req, fuse2_ino_t ino, size_t blocksize,
		     uint64_t idx);
	void (*ioctl)(fuse2_req_t req, fuse2_ino_t ino, int cmd, void *arg,
		      struct fuse2_file_info *fi, unsigned int flags,
		      const void *in_buf, size_t in_bufsz, size_t out_bufsz);
	void (*poll)(fuse2_req_t req, fuse2_ino_t ino, struct fuse2_file_info *fi,
		     struct fuse2_pollhandle *ph);
	void (*write_buf)(fuse2_req_t req, fuse2_ino_t ino,
			  struct fuse2_bufvec *bufv, off_t off,
			  struct fuse2_file_info *fi);
	void (*retrieve_reply)(fuse2_req_t req, void *cookie, fuse2_ino_t ino,
			       off_t offset, struct fuse2_bufvec *bufv);
	void (*forget_multi)(fuse2_req_t req, size_t count,
			     struct fuse2_forget_data *forgets);
	void (*flock)(fuse2_req_t req, fuse2_ino_t ino, struct fuse2_file_info *fi,
		      int op);
	void (*fallocate)(fuse2_req_t req, fuse2_ino_t ino, int mode,
			  off_t offset, off_t length, struct fuse2_file_info *fi);
};

#endif
