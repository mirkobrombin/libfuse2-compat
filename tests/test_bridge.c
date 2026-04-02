/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "fuse2_exports.h"

#include <dlfcn.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct test_state {
	unsigned int init;
	unsigned int destroy;
	unsigned int callbacks;
};

struct fake_fuse3_counters {
	unsigned int mounts;
	unsigned int unmounts;
	unsigned int loops;
	unsigned int replies;
	unsigned int failures;
};

static struct test_state state;

static void check(bool condition, const char *message)
{
	if (condition)
		return;
	fprintf(stderr, "test-bridge: %s\n", message);
	exit(EXIT_FAILURE);
}

static void legacy_init(void *userdata, struct fuse2_conn_info *conn)
{
	check(userdata == &state, "init userdata mismatch");
	check(conn->proto_major == 7 && conn->proto_minor == 31,
	      "conn protocol conversion failed");
	check(conn->max_write == 131072, "conn max_write conversion failed");
	check(conn->async_read == 1U, "conn async_read conversion failed");
	conn->max_write = 262144;
	conn->async_read = 1;
	state.init++;
}

static void legacy_destroy(void *userdata)
{
	check(userdata == &state, "destroy userdata mismatch");
	state.destroy++;
}

static void legacy_lookup(fuse2_req_t req, fuse2_ino_t parent,
			  const char *name)
{
	struct fuse2_entry_param entry;

	check(fuse_req_userdata(req) == &state, "request userdata mismatch");
	check(parent == 1 && strcmp(name, "hello") == 0, "lookup conversion failed");
	memset(&entry, 0, sizeof(entry));
	entry.ino = 2;
	entry.generation = 3;
	entry.attr.st_ino = 2;
	entry.attr.st_mode = S_IFREG | 0444;
	entry.attr_timeout = 1.0;
	entry.entry_timeout = 1.0;
	check(fuse_reply_entry(req, &entry) == 0, "reply_entry failed");
	state.callbacks++;
}

static void legacy_forget(fuse2_req_t req, fuse2_ino_t ino,
			  unsigned long nlookup)
{
	check(ino == 2 && nlookup == 1, "forget conversion failed");
	fuse_reply_none(req);
	state.callbacks++;
}

static void legacy_getattr(fuse2_req_t req, fuse2_ino_t ino,
			   struct fuse2_file_info *fi)
{
	struct stat attr;

	check(ino == 2 && fi != NULL, "getattr conversion failed");
	check(fi->writepage == 1 && fi->fh == 42, "getattr file_info mismatch");
	memset(&attr, 0, sizeof(attr));
	attr.st_ino = 2;
	attr.st_mode = S_IFREG | 0444;
	check(fuse_reply_attr(req, &attr, 1.0) == 0, "reply_attr failed");
	state.callbacks++;
}

static void legacy_open(fuse2_req_t req, fuse2_ino_t ino,
			struct fuse2_file_info *fi)
{
	check(ino == 2 && fi->fh == 42 && fi->writepage == 1,
	      "open file_info conversion failed");
	fi->fh = 99;
	fi->keep_cache = 1;
	check(fuse_reply_open(req, fi) == 0, "reply_open failed");
	state.callbacks++;
}

static void legacy_read(fuse2_req_t req, fuse2_ino_t ino, size_t size,
			off_t off, struct fuse2_file_info *fi)
{
	check(ino == 2 && size == 7 && off == 0 && fi->fh == 99,
	      "read conversion failed");
	check(fuse_reply_buf(req, "payload", 7) == 0, "reply_buf failed");
	state.callbacks++;
}

static void legacy_release(fuse2_req_t req, fuse2_ino_t ino,
			   struct fuse2_file_info *fi)
{
	check(ino == 2 && fi->fh == 99, "release conversion failed");
	check(fuse_reply_err(req, 0) == 0, "release reply failed");
	state.callbacks++;
}

static void legacy_opendir(fuse2_req_t req, fuse2_ino_t ino,
			   struct fuse2_file_info *fi)
{
	check(ino == 1, "opendir inode mismatch");
	fi->fh = 100;
	check(fuse_reply_open(req, fi) == 0, "opendir reply failed");
	state.callbacks++;
}

static void legacy_readdir(fuse2_req_t req, fuse2_ino_t ino, size_t size,
			   off_t off, struct fuse2_file_info *fi)
{
	char buffer[32];
	struct stat attr;
	size_t used;

	check(ino == 1 && size == 128 && off == 0 && fi->fh == 100,
	      "readdir conversion failed");
	memset(&attr, 0, sizeof(attr));
	used = fuse_add_direntry(req, buffer, sizeof(buffer), "hello", &attr, 1);
	check(used == 6, "add_direntry failed");
	check(fuse_reply_buf(req, buffer, used) == 0, "readdir reply failed");
	state.callbacks++;
}

static void legacy_releasedir(fuse2_req_t req, fuse2_ino_t ino,
			      struct fuse2_file_info *fi)
{
	check(ino == 1 && fi->fh == 100, "releasedir conversion failed");
	check(fuse_reply_err(req, 0) == 0, "releasedir reply failed");
	state.callbacks++;
}

static void legacy_statfs(fuse2_req_t req, fuse2_ino_t ino)
{
	struct statvfs value;

	check(ino == 1, "statfs inode mismatch");
	memset(&value, 0, sizeof(value));
	check(fuse_reply_statfs(req, &value) == 0, "statfs reply failed");
	state.callbacks++;
}

static void legacy_getxattr(fuse2_req_t req, fuse2_ino_t ino,
			    const char *name, size_t size)
{
	check(ino == 2 && strcmp(name, "user.test") == 0 && size == 0,
	      "getxattr conversion failed");
	check(fuse_reply_xattr(req, 4) == 0, "xattr reply failed");
	state.callbacks++;
}

static void legacy_listxattr(fuse2_req_t req, fuse2_ino_t ino, size_t size)
{
	check(ino == 2 && size == 32, "listxattr conversion failed");
	check(fuse_reply_buf(req, "user.test", 10) == 0, "listxattr reply failed");
	state.callbacks++;
}

static void legacy_create(fuse2_req_t req, fuse2_ino_t parent,
			  const char *name, mode_t mode,
			  struct fuse2_file_info *fi)
{
	(void)fi;
	check(parent == 1 && strcmp(name, "forbidden") == 0 && mode == 0644,
	      "create conversion failed");
	check(fuse_reply_err(req, EROFS) == 0, "create reply failed");
	state.callbacks++;
}

static const struct fake_fuse3_counters *backend_counters(void)
{
	const char *path = getenv("LIBFUSE2_COMPAT_BACKEND");
	const struct fake_fuse3_counters *(*get_counters)(void);
	void *handle;
	void *symbol;

	check(path != NULL, "backend path missing");
	handle = dlopen(path, RTLD_NOW | RTLD_NOLOAD);
	check(handle != NULL, "fake backend is not loaded");
	symbol = dlsym(handle, "fuse2compat_test_backend_counters");
	check(symbol != NULL, "fake backend counter symbol missing");
	memcpy(&get_counters, &symbol, sizeof(symbol));
	return get_counters();
}

int main(void)
{
	char *argv[] = { "legacy-test", "-s", "/tmp/fuse2-compat-test", NULL };
	struct fuse2_args args = { 3, argv, 0 };
	struct fuse2_lowlevel_ops ops;
	struct fuse2_session *session;
	struct fuse2_chan *chan;
	const struct fake_fuse3_counters *counters;
	char *mountpoint = NULL;
	int multithreaded = 1;
	int foreground = 0;

	memset(&ops, 0, sizeof(ops));
	ops.init = legacy_init;
	ops.destroy = legacy_destroy;
	ops.lookup = legacy_lookup;
	ops.forget = legacy_forget;
	ops.getattr = legacy_getattr;
	ops.open = legacy_open;
	ops.read = legacy_read;
	ops.release = legacy_release;
	ops.opendir = legacy_opendir;
	ops.readdir = legacy_readdir;
	ops.releasedir = legacy_releasedir;
	ops.statfs = legacy_statfs;
	ops.getxattr = legacy_getxattr;
	ops.listxattr = legacy_listxattr;
	ops.create = legacy_create;

	check(fuse_version() == 29, "unexpected compatibility version");
	check(fuse_parse_cmdline(&args, &mountpoint, &multithreaded,
				 &foreground) == 0,
	      "parse_cmdline failed");
	check(multithreaded == 0 && foreground == 0,
	      "parse_cmdline flags mismatch");
	chan = fuse_mount(mountpoint, &args);
	check(chan != NULL, "fuse_mount failed");
	session = fuse_lowlevel_new(&args, &ops, sizeof(ops), &state);
	check(session != NULL, "fuse_lowlevel_new failed");
	check(fuse_set_signal_handlers(session) == 0, "signal setup failed");
	fuse_session_add_chan(session, chan);
	check(fuse_session_loop(session) == 0, "session loop failed");
	fuse_remove_signal_handlers(session);
	fuse_session_remove_chan(chan);
	fuse_session_destroy(session);
	fuse_unmount(mountpoint, chan);
	fuse_opt_free_args(&args);
	free(mountpoint);

	counters = backend_counters();
	check(state.init == 1 && state.destroy == 1, "lifecycle callbacks mismatch");
	check(state.callbacks == 13, "operation callback count mismatch");
	check(counters->mounts == 1 && counters->unmounts == 1 &&
	      counters->loops == 1, "backend lifecycle mismatch");
	check(counters->replies == 13, "backend reply count mismatch");
	check(counters->failures == 0, "backend conversion checks failed");

	puts("bridge unit test: ok");
	return EXIT_SUCCESS;
}
