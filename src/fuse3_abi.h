/* SPDX-License-Identifier: LGPL-2.1-or-later */
#ifndef LIBFUSE2_COMPAT_FUSE3_ABI_H
#define LIBFUSE2_COMPAT_FUSE3_ABI_H

#include <fcntl.h>
#include <stdint.h>
#include <stddef.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/types.h>

typedef uint64_t fuse3_ino_t;
typedef struct fuse3_req *fuse3_req_t;

struct fuse3_session;
struct fuse3_pollhandle;

struct fuse3_args {
	int argc;
	char **argv;
	int allocated;
};

struct fuse3_opt {
	const char *templ;
	unsigned long offset;
	int value;
};

typedef int (*fuse3_opt_proc_t)(void *data, const char *arg, int key,
				struct fuse3_args *outargs);

struct fuse3_cmdline_opts {
	int singlethread;
	int foreground;
	int debug;
	int nodefault_subtype;
	char *mountpoint;
	int show_version;
	int show_help;
	int clone_fd;
	unsigned int max_idle_threads;
	unsigned int max_threads;
};

/* This is the stable FUSE 3.0 ABI layout used by fuse_session_new(). */
struct fuse3_file_info {
	int flags;
	unsigned int writepage : 1;
	unsigned int direct_io : 1;
	unsigned int keep_cache : 1;
	unsigned int flush : 1;
	unsigned int nonseekable : 1;
	unsigned int flock_release : 1;
	unsigned int cache_readdir : 1;
	unsigned int padding : 25;
	unsigned int padding2;
	uint64_t fh;
	uint64_t lock_owner;
	uint32_t poll_events;
};

struct fuse3_conn_info {
	unsigned int proto_major;
	unsigned int proto_minor;
	unsigned int max_write;
	unsigned int max_read;
	unsigned int max_readahead;
	unsigned int capable;
	unsigned int want;
	unsigned int max_background;
	unsigned int congestion_threshold;
	unsigned int time_gran;
	unsigned int reserved[22];
};

struct fuse3_entry_param {
	fuse3_ino_t ino;
	uint64_t generation;
	struct stat attr;
	double attr_timeout;
	double entry_timeout;
};

enum fuse3_buf_flags {
	FUSE3_BUF_IS_FD = 1 << 1,
	FUSE3_BUF_FD_SEEK = 1 << 2,
	FUSE3_BUF_FD_RETRY = 1 << 3,
};

struct fuse3_buf {
	size_t size;
	enum fuse3_buf_flags flags;
	void *mem;
	int fd;
	off_t pos;
};

struct fuse3_bufvec {
	size_t count;
	size_t idx;
	size_t off;
	struct fuse3_buf buf[1];
};

struct fuse3_forget_data {
	fuse3_ino_t ino;
	uint64_t nlookup;
};

/* Prefix through FUSE 3.0's fallocate callback. op_size gates the suffix. */
struct fuse3_lowlevel_ops {
	void (*init)(void *userdata, struct fuse3_conn_info *conn);
	void (*destroy)(void *userdata);
	void (*lookup)(fuse3_req_t req, fuse3_ino_t parent, const char *name);
	void (*forget)(fuse3_req_t req, fuse3_ino_t ino, uint64_t nlookup);
	void (*getattr)(fuse3_req_t req, fuse3_ino_t ino, struct fuse3_file_info *fi);
	void (*setattr)(fuse3_req_t req, fuse3_ino_t ino, struct stat *attr,
		       int to_set, struct fuse3_file_info *fi);
	void (*readlink)(fuse3_req_t req, fuse3_ino_t ino);
	void (*mknod)(fuse3_req_t req, fuse3_ino_t parent, const char *name,
		      mode_t mode, dev_t rdev);
	void (*mkdir)(fuse3_req_t req, fuse3_ino_t parent, const char *name,
		      mode_t mode);
	void (*unlink)(fuse3_req_t req, fuse3_ino_t parent, const char *name);
	void (*rmdir)(fuse3_req_t req, fuse3_ino_t parent, const char *name);
	void (*symlink)(fuse3_req_t req, const char *link, fuse3_ino_t parent,
			const char *name);
	void (*rename)(fuse3_req_t req, fuse3_ino_t parent, const char *name,
		       fuse3_ino_t newparent, const char *newname,
		       unsigned int flags);
	void (*link)(fuse3_req_t req, fuse3_ino_t ino, fuse3_ino_t newparent,
		     const char *newname);
	void (*open)(fuse3_req_t req, fuse3_ino_t ino, struct fuse3_file_info *fi);
	void (*read)(fuse3_req_t req, fuse3_ino_t ino, size_t size, off_t off,
		     struct fuse3_file_info *fi);
	void (*write)(fuse3_req_t req, fuse3_ino_t ino, const char *buf,
		      size_t size, off_t off, struct fuse3_file_info *fi);
	void (*flush)(fuse3_req_t req, fuse3_ino_t ino, struct fuse3_file_info *fi);
	void (*release)(fuse3_req_t req, fuse3_ino_t ino, struct fuse3_file_info *fi);
	void (*fsync)(fuse3_req_t req, fuse3_ino_t ino, int datasync,
		      struct fuse3_file_info *fi);
	void (*opendir)(fuse3_req_t req, fuse3_ino_t ino, struct fuse3_file_info *fi);
	void (*readdir)(fuse3_req_t req, fuse3_ino_t ino, size_t size, off_t off,
			struct fuse3_file_info *fi);
	void (*releasedir)(fuse3_req_t req, fuse3_ino_t ino,
			   struct fuse3_file_info *fi);
	void (*fsyncdir)(fuse3_req_t req, fuse3_ino_t ino, int datasync,
			 struct fuse3_file_info *fi);
	void (*statfs)(fuse3_req_t req, fuse3_ino_t ino);
	void (*setxattr)(fuse3_req_t req, fuse3_ino_t ino, const char *name,
			 const char *value, size_t size, int flags);
	void (*getxattr)(fuse3_req_t req, fuse3_ino_t ino, const char *name,
			 size_t size);
	void (*listxattr)(fuse3_req_t req, fuse3_ino_t ino, size_t size);
	void (*removexattr)(fuse3_req_t req, fuse3_ino_t ino, const char *name);
	void (*access)(fuse3_req_t req, fuse3_ino_t ino, int mask);
	void (*create)(fuse3_req_t req, fuse3_ino_t parent, const char *name,
		       mode_t mode, struct fuse3_file_info *fi);
	void (*getlk)(fuse3_req_t req, fuse3_ino_t ino, struct fuse3_file_info *fi,
		      struct flock *lock);
	void (*setlk)(fuse3_req_t req, fuse3_ino_t ino, struct fuse3_file_info *fi,
		      struct flock *lock, int sleep);
	void (*bmap)(fuse3_req_t req, fuse3_ino_t ino, size_t blocksize,
		     uint64_t idx);
	void (*ioctl)(fuse3_req_t req, fuse3_ino_t ino, int cmd, void *arg,
		      struct fuse3_file_info *fi, unsigned int flags,
		      const void *in_buf, size_t in_bufsz, size_t out_bufsz);
	void (*poll)(fuse3_req_t req, fuse3_ino_t ino, struct fuse3_file_info *fi,
		     struct fuse3_pollhandle *ph);
	void (*write_buf)(fuse3_req_t req, fuse3_ino_t ino,
			  struct fuse3_bufvec *bufv, off_t off,
			  struct fuse3_file_info *fi);
	void (*retrieve_reply)(fuse3_req_t req, void *cookie, fuse3_ino_t ino,
			       off_t offset, struct fuse3_bufvec *bufv);
	void (*forget_multi)(fuse3_req_t req, size_t count,
			     struct fuse3_forget_data *forgets);
	void (*flock)(fuse3_req_t req, fuse3_ino_t ino, struct fuse3_file_info *fi,
		      int op);
	void (*fallocate)(fuse3_req_t req, fuse3_ino_t ino, int mode,
			  off_t offset, off_t length, struct fuse3_file_info *fi);
};

#endif
