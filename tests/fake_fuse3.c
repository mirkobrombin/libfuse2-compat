/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "fuse3_abi.h"

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct fuse3_session {
	struct fuse3_lowlevel_ops ops;
	void *userdata;
	bool mounted;
	bool exited;
};

struct fuse3_req {
	void *userdata;
};

struct fake_fuse3_counters {
	unsigned int mounts;
	unsigned int unmounts;
	unsigned int loops;
	unsigned int replies;
	unsigned int failures;
};

static struct fake_fuse3_counters counters;

static void fail_if(bool condition, const char *message)
{
	if (!condition)
		return;
	fprintf(stderr, "fake-fuse3: %s\n", message);
	counters.failures++;
}

int fuse_opt_parse(struct fuse3_args *args, void *data,
		   const struct fuse3_opt opts[], fuse3_opt_proc_t proc)
{
	(void)args;
	(void)data;
	(void)opts;
	(void)proc;
	return 0;
}

int fuse_opt_add_arg(struct fuse3_args *args, const char *arg)
{
	char **new_argv;
	int index;

	if (args == NULL || arg == NULL)
		return -1;
	new_argv = calloc((size_t)args->argc + 2U, sizeof(*new_argv));
	if (new_argv == NULL)
		return -1;
	for (index = 0; index < args->argc; index++) {
		new_argv[index] = strdup(args->argv[index]);
		if (new_argv[index] == NULL)
			return -1;
	}
	new_argv[args->argc] = strdup(arg);
	if (new_argv[args->argc] == NULL)
		return -1;
	if (args->allocated != 0) {
		for (index = 0; index < args->argc; index++)
			free(args->argv[index]);
		free(args->argv);
	}
	args->argv = new_argv;
	args->argc++;
	args->allocated = 1;
	return 0;
}

void fuse_opt_free_args(struct fuse3_args *args)
{
	int index;

	if (args == NULL || args->allocated == 0)
		return;
	for (index = 0; index < args->argc; index++)
		free(args->argv[index]);
	free(args->argv);
	args->argv = NULL;
	args->argc = 0;
	args->allocated = 0;
}

int fuse_parse_cmdline(struct fuse3_args *args,
		       struct fuse3_cmdline_opts *opts)
{
	int index;

	if (args == NULL || opts == NULL)
		return -1;
	memset(opts, 0, sizeof(*opts));
	for (index = 1; index < args->argc; index++) {
		if (strcmp(args->argv[index], "-s") == 0)
			opts->singlethread = 1;
		else if (strcmp(args->argv[index], "-f") == 0)
			opts->foreground = 1;
		else if (args->argv[index][0] != '-')
			opts->mountpoint = strdup(args->argv[index]);
	}
	return opts->mountpoint != NULL ? 0 : -1;
}

struct fuse3_session *fuse_session_new(
	struct fuse3_args *args, const struct fuse3_lowlevel_ops *ops,
	size_t op_size, void *userdata)
{
	struct fuse3_session *session;
	size_t copy_size;

	(void)args;
	if (ops == NULL)
		return NULL;
	session = calloc(1, sizeof(*session));
	if (session == NULL)
		return NULL;
	copy_size = op_size < sizeof(session->ops) ? op_size : sizeof(session->ops);
	memcpy(&session->ops, ops, copy_size);
	session->userdata = userdata;
	return session;
}

int fuse_session_mount(struct fuse3_session *session, const char *mountpoint)
{
	fail_if(session == NULL, "null session passed to mount");
	fail_if(mountpoint == NULL || strcmp(mountpoint, "/tmp/fuse2-compat-test") != 0,
		"mountpoint conversion failed");
	if (session == NULL)
		return -1;
	session->mounted = true;
	counters.mounts++;
	return 0;
}

void fuse_session_unmount(struct fuse3_session *session)
{
	fail_if(session == NULL || !session->mounted, "invalid unmount");
	if (session != NULL)
		session->mounted = false;
	counters.unmounts++;
}

int fuse_session_loop(struct fuse3_session *session)
{
	struct fuse3_conn_info conn;
	struct fuse3_req req;
	struct fuse3_file_info fi;

	if (session == NULL || !session->mounted)
		return -EIO;
	counters.loops++;
	memset(&conn, 0, sizeof(conn));
	conn.proto_major = 7;
	conn.proto_minor = 31;
	conn.max_write = 131072;
	conn.max_readahead = 65536;
	conn.capable = 1U;
	conn.max_background = 12;
	conn.congestion_threshold = 8;
	if (session->ops.init != NULL)
		session->ops.init(session->userdata, &conn);
	fail_if(conn.max_write != 262144, "conn_info max_write was not copied back");
	fail_if((conn.want & 1U) == 0U, "conn_info async_read was not copied back");

	req.userdata = session->userdata;
	if (session->ops.lookup != NULL)
		session->ops.lookup(&req, 1, "hello");

	memset(&fi, 0, sizeof(fi));
	fi.flags = O_RDONLY;
	fi.writepage = 1;
	fi.fh = 42;
	if (session->ops.getattr != NULL)
		session->ops.getattr(&req, 2, &fi);
	if (session->ops.open != NULL)
		session->ops.open(&req, 2, &fi);
	fail_if(fi.fh != 99, "file_info fh was not copied back");
	fail_if(fi.keep_cache == 0U, "file_info keep_cache was not copied back");
	if (session->ops.read != NULL)
		session->ops.read(&req, 2, 7, 0, &fi);
	if (session->ops.release != NULL)
		session->ops.release(&req, 2, &fi);

	memset(&fi, 0, sizeof(fi));
	if (session->ops.opendir != NULL)
		session->ops.opendir(&req, 1, &fi);
	if (session->ops.readdir != NULL)
		session->ops.readdir(&req, 1, 128, 0, &fi);
	if (session->ops.releasedir != NULL)
		session->ops.releasedir(&req, 1, &fi);
	if (session->ops.statfs != NULL)
		session->ops.statfs(&req, 1);
	if (session->ops.getxattr != NULL)
		session->ops.getxattr(&req, 2, "user.test", 0);
	if (session->ops.listxattr != NULL)
		session->ops.listxattr(&req, 2, 32);
	if (session->ops.create != NULL)
		session->ops.create(&req, 1, "forbidden", 0644, &fi);
	if (session->ops.forget != NULL)
		session->ops.forget(&req, 2, 1);

	return counters.failures == 0U ? 0 : -EIO;
}

void fuse_session_destroy(struct fuse3_session *session)
{
	if (session == NULL)
		return;
	if (session->ops.destroy != NULL)
		session->ops.destroy(session->userdata);
	free(session);
}

void fuse_session_exit(struct fuse3_session *session)
{
	if (session != NULL)
		session->exited = true;
}

int fuse_session_exited(struct fuse3_session *session)
{
	return session != NULL && session->exited ? 1 : 0;
}

void fuse_session_reset(struct fuse3_session *session)
{
	if (session != NULL)
		session->exited = false;
}

int fuse_set_signal_handlers(struct fuse3_session *session)
{
	return session != NULL ? 0 : -1;
}

void fuse_remove_signal_handlers(struct fuse3_session *session)
{
	(void)session;
}

int fuse_daemonize(int foreground)
{
	return foreground >= 0 ? 0 : -1;
}

void *fuse_req_userdata(fuse3_req_t req)
{
	return req != NULL ? req->userdata : NULL;
}

int fuse_reply_err(fuse3_req_t req, int err)
{
	(void)req;
	(void)err;
	counters.replies++;
	return 0;
}

void fuse_reply_none(fuse3_req_t req)
{
	(void)req;
	counters.replies++;
}

int fuse_reply_entry(fuse3_req_t req, const struct fuse3_entry_param *entry)
{
	(void)req;
	fail_if(entry == NULL || entry->ino != 2 || entry->generation != 3,
		"entry_param conversion failed");
	counters.replies++;
	return 0;
}

int fuse_reply_attr(fuse3_req_t req, const struct stat *attr,
		    double attr_timeout)
{
	(void)req;
	fail_if(attr == NULL || attr->st_ino != 2 || attr_timeout != 1.0,
		"attr reply conversion failed");
	counters.replies++;
	return 0;
}

int fuse_reply_open(fuse3_req_t req, const struct fuse3_file_info *fi)
{
	(void)req;
	fail_if(fi == NULL || (fi->fh != 99 && fi->fh != 100),
		"open reply conversion failed");
	counters.replies++;
	return 0;
}

int fuse_reply_buf(fuse3_req_t req, const char *buf, size_t size)
{
	(void)req;
	(void)buf;
	(void)size;
	counters.replies++;
	return 0;
}

int fuse_reply_readlink(fuse3_req_t req, const char *link)
{
	(void)req;
	(void)link;
	counters.replies++;
	return 0;
}

int fuse_reply_xattr(fuse3_req_t req, size_t count)
{
	(void)req;
	(void)count;
	counters.replies++;
	return 0;
}

int fuse_reply_write(fuse3_req_t req, size_t count)
{
	(void)req;
	(void)count;
	counters.replies++;
	return 0;
}

int fuse_reply_statfs(fuse3_req_t req, const struct statvfs *statfsbuf)
{
	(void)req;
	(void)statfsbuf;
	counters.replies++;
	return 0;
}

int fuse_reply_create(fuse3_req_t req, const struct fuse3_entry_param *entry,
		      const struct fuse3_file_info *fi)
{
	(void)req;
	(void)entry;
	(void)fi;
	counters.replies++;
	return 0;
}

int fuse_reply_lock(fuse3_req_t req, const struct flock *lock)
{
	(void)req;
	(void)lock;
	counters.replies++;
	return 0;
}

int fuse_reply_bmap(fuse3_req_t req, uint64_t idx)
{
	(void)req;
	(void)idx;
	counters.replies++;
	return 0;
}

size_t fuse_add_direntry(fuse3_req_t req, char *buf, size_t bufsize,
			 const char *name, const struct stat *statbuf, off_t off)
{
	size_t needed = strlen(name) + 1U;

	(void)req;
	(void)statbuf;
	(void)off;
	if (buf != NULL && bufsize >= needed)
		memcpy(buf, name, needed);
	return needed;
}

const struct fake_fuse3_counters *fuse2compat_test_backend_counters(void)
{
	return &counters;
}
