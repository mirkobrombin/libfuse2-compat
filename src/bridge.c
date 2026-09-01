/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "fuse2_exports.h"
#include "bridge_native.h"
#include "fuse3_loader.h"

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FUSE2_CAP_ASYNC_READ (1U << 0)
#define SESSION_MAGIC UINT64_C(0x463243534553534e)
#define CHANNEL_MAGIC UINT64_C(0x4632434348414e4c)

struct fuse2_session {
	uint64_t magic;
	struct fuse3_session *backend;
	struct fuse2_lowlevel_ops ops;
	void *userdata;
	struct fuse2_chan *chan;
	int mount_error;
};

struct fuse2_chan {
	uint64_t magic;
	char *mountpoint;
	struct fuse2_session *session;
	bool mounted;
};

static const struct fuse3_api *require_api(void)
{
	const struct fuse3_api *api = fuse3_api_get();

	if (api == NULL) {
		const char *message = fuse3_api_error();
		fprintf(stderr, "libfuse2-compat: %s\n",
			message != NULL ? message : "FUSE3 backend unavailable");
		errno = ENOSYS;
	}
	return api;
}

static bool valid_session(const struct fuse2_session *session)
{
	return session != NULL && session->magic == SESSION_MAGIC &&
	       session->backend != NULL;
}

static bool valid_chan(const struct fuse2_chan *chan)
{
	return chan != NULL && chan->magic == CHANNEL_MAGIC &&
	       chan->mountpoint != NULL;
}

static char *duplicate_string(const char *value)
{
	size_t size = strlen(value) + 1U;
	char *copy = malloc(size);

	if (copy != NULL)
		memcpy(copy, value, size);
	return copy;
}

static struct fuse2_session *session_from_req(fuse3_req_t req)
{
	const struct fuse3_api *api = fuse3_api_get();
	struct fuse2_session *session;

	if (api == NULL)
		return NULL;
	session = api->req_userdata(req);
	return valid_session(session) ? session : NULL;
}

static void file_info_3_to_2(const struct fuse3_file_info *source,
			     struct fuse2_file_info *destination)
{
	memset(destination, 0, sizeof(*destination));
	if (source == NULL)
		return;
	destination->flags = source->flags;
	destination->writepage = (int)source->writepage;
	destination->direct_io = source->direct_io;
	destination->keep_cache = source->keep_cache;
	destination->flush = source->flush;
	destination->nonseekable = source->nonseekable;
	destination->flock_release = source->flock_release;
	destination->fh = source->fh;
	destination->lock_owner = source->lock_owner;
}

static void file_info_2_to_3(const struct fuse2_file_info *source,
			     struct fuse3_file_info *destination)
{
	if (source == NULL || destination == NULL)
		return;
	destination->flags = source->flags;
	destination->writepage = source->writepage != 0;
	destination->direct_io = source->direct_io;
	destination->keep_cache = source->keep_cache;
	destination->flush = source->flush;
	destination->nonseekable = source->nonseekable;
	destination->flock_release = source->flock_release;
	destination->fh = source->fh;
	destination->lock_owner = source->lock_owner;
}

static void conn_info_3_to_2(const struct fuse3_conn_info *source,
			     struct fuse2_conn_info *destination)
{
	memset(destination, 0, sizeof(*destination));
	destination->proto_major = source->proto_major;
	destination->proto_minor = source->proto_minor;
	destination->async_read =
		(source->capable & FUSE2_CAP_ASYNC_READ) != 0U ? 1U : 0U;
	destination->max_write = source->max_write;
	destination->max_readahead = source->max_readahead;
	destination->capable = source->capable;
	destination->want = source->want;
	destination->max_background = source->max_background;
	destination->congestion_threshold = source->congestion_threshold;
}

static void conn_info_2_to_3(const struct fuse2_conn_info *source,
			     struct fuse3_conn_info *destination)
{
	destination->max_write = source->max_write;
	destination->max_readahead = source->max_readahead;
	destination->want = source->want;
	if (source->async_read != 0U)
		destination->want |= FUSE2_CAP_ASYNC_READ;
	else
		destination->want &= ~FUSE2_CAP_ASYNC_READ;
	destination->max_background = source->max_background;
	destination->congestion_threshold = source->congestion_threshold;
}

static void entry_2_to_3(const struct fuse2_entry_param *source,
			 struct fuse3_entry_param *destination)
{
	memset(destination, 0, sizeof(*destination));
	destination->ino = (fuse3_ino_t)source->ino;
	destination->generation = (uint64_t)source->generation;
	destination->attr = source->attr;
	destination->attr_timeout = source->attr_timeout;
	destination->entry_timeout = source->entry_timeout;
}

static void bridge_init(void *userdata, struct fuse3_conn_info *conn)
{
	struct fuse2_session *session = userdata;
	struct fuse2_conn_info legacy;

	if (!valid_session(session) || session->ops.init == NULL)
		return;
	conn_info_3_to_2(conn, &legacy);
	fuse2_foundation_bridge_init(session->ops.init, session->userdata, &legacy);
	conn_info_2_to_3(&legacy, conn);
}

static void bridge_destroy(void *userdata)
{
	struct fuse2_session *session = userdata;

	if (valid_session(session) && session->ops.destroy != NULL)
		fuse2_foundation_bridge_destroy(session->ops.destroy,
						session->userdata);
}

static void bridge_lookup(fuse3_req_t req, fuse3_ino_t parent,
			  const char *name)
{
	struct fuse2_session *session = session_from_req(req);

	if (session != NULL)
		fuse2_foundation_bridge_lookup(session->ops.lookup,
						(fuse2_req_t)req,
						(fuse2_ino_t)parent, name);
}

static void bridge_forget(fuse3_req_t req, fuse3_ino_t ino, uint64_t nlookup)
{
	struct fuse2_session *session = session_from_req(req);

	if (session != NULL)
		fuse2_foundation_bridge_forget(session->ops.forget,
						(fuse2_req_t)req,
						(fuse2_ino_t)ino,
						(unsigned long)nlookup);
}

static void bridge_getattr(fuse3_req_t req, fuse3_ino_t ino,
			   struct fuse3_file_info *fi)
{
	struct fuse2_session *session = session_from_req(req);
	struct fuse2_file_info legacy;
	struct fuse2_file_info *legacy_ptr = fi != NULL ? &legacy : NULL;

	if (session == NULL)
		return;
	file_info_3_to_2(fi, &legacy);
	fuse2_foundation_bridge_getattr(session->ops.getattr, (fuse2_req_t)req,
					(fuse2_ino_t)ino, legacy_ptr);
	file_info_2_to_3(legacy_ptr, fi);
}

static void bridge_setattr(fuse3_req_t req, fuse3_ino_t ino,
			   struct stat *attr, int to_set,
			   struct fuse3_file_info *fi)
{
	struct fuse2_session *session = session_from_req(req);
	struct fuse2_file_info legacy;
	struct fuse2_file_info *legacy_ptr = fi != NULL ? &legacy : NULL;

	if (session == NULL)
		return;
	file_info_3_to_2(fi, &legacy);
	fuse2_foundation_bridge_setattr(session->ops.setattr, (fuse2_req_t)req,
					(fuse2_ino_t)ino, attr, to_set,
					legacy_ptr);
	file_info_2_to_3(legacy_ptr, fi);
}

static void bridge_readlink(fuse3_req_t req, fuse3_ino_t ino)
{
	struct fuse2_session *session = session_from_req(req);

	if (session != NULL)
		fuse2_foundation_bridge_readlink(session->ops.readlink,
						  (fuse2_req_t)req,
						  (fuse2_ino_t)ino);
}

static void bridge_open(fuse3_req_t req, fuse3_ino_t ino,
			struct fuse3_file_info *fi)
{
	struct fuse2_session *session = session_from_req(req);
	struct fuse2_file_info legacy;

	if (session == NULL)
		return;
	file_info_3_to_2(fi, &legacy);
	fuse2_foundation_bridge_open(session->ops.open, (fuse2_req_t)req,
				     (fuse2_ino_t)ino, &legacy);
	file_info_2_to_3(&legacy, fi);
}

static void bridge_read(fuse3_req_t req, fuse3_ino_t ino, size_t size,
			off_t off, struct fuse3_file_info *fi)
{
	struct fuse2_session *session = session_from_req(req);
	struct fuse2_file_info legacy;

	if (session == NULL)
		return;
	file_info_3_to_2(fi, &legacy);
	fuse2_foundation_bridge_read(session->ops.read, (fuse2_req_t)req,
				     (fuse2_ino_t)ino, size, off, &legacy);
	file_info_2_to_3(&legacy, fi);
}

static void bridge_write(fuse3_req_t req, fuse3_ino_t ino, const char *buf,
			 size_t size, off_t off, struct fuse3_file_info *fi)
{
	struct fuse2_session *session = session_from_req(req);
	struct fuse2_file_info legacy;

	if (session == NULL)
		return;
	file_info_3_to_2(fi, &legacy);
	fuse2_foundation_bridge_write(session->ops.write, (fuse2_req_t)req,
				      (fuse2_ino_t)ino, buf, size, off,
				      &legacy);
	file_info_2_to_3(&legacy, fi);
}

static void bridge_release(fuse3_req_t req, fuse3_ino_t ino,
			   struct fuse3_file_info *fi)
{
	struct fuse2_session *session = session_from_req(req);
	struct fuse2_file_info legacy;

	if (session == NULL)
		return;
	file_info_3_to_2(fi, &legacy);
	fuse2_foundation_bridge_release(session->ops.release, (fuse2_req_t)req,
					(fuse2_ino_t)ino, &legacy);
	file_info_2_to_3(&legacy, fi);
}

static void bridge_opendir(fuse3_req_t req, fuse3_ino_t ino,
			   struct fuse3_file_info *fi)
{
	struct fuse2_session *session = session_from_req(req);
	struct fuse2_file_info legacy;

	if (session == NULL)
		return;
	file_info_3_to_2(fi, &legacy);
	fuse2_foundation_bridge_opendir(session->ops.opendir, (fuse2_req_t)req,
					(fuse2_ino_t)ino, &legacy);
	file_info_2_to_3(&legacy, fi);
}

static void bridge_readdir(fuse3_req_t req, fuse3_ino_t ino, size_t size,
			   off_t off, struct fuse3_file_info *fi)
{
	struct fuse2_session *session = session_from_req(req);
	struct fuse2_file_info legacy;

	if (session == NULL)
		return;
	file_info_3_to_2(fi, &legacy);
	fuse2_foundation_bridge_readdir(session->ops.readdir, (fuse2_req_t)req,
					(fuse2_ino_t)ino, size, off,
					&legacy);
	file_info_2_to_3(&legacy, fi);
}

static void bridge_releasedir(fuse3_req_t req, fuse3_ino_t ino,
			      struct fuse3_file_info *fi)
{
	struct fuse2_session *session = session_from_req(req);
	struct fuse2_file_info legacy;

	if (session == NULL)
		return;
	file_info_3_to_2(fi, &legacy);
	fuse2_foundation_bridge_releasedir(session->ops.releasedir,
					   (fuse2_req_t)req,
					   (fuse2_ino_t)ino, &legacy);
	file_info_2_to_3(&legacy, fi);
}

static void bridge_statfs(fuse3_req_t req, fuse3_ino_t ino)
{
	struct fuse2_session *session = session_from_req(req);

	if (session != NULL)
		fuse2_foundation_bridge_statfs(session->ops.statfs,
						(fuse2_req_t)req,
						(fuse2_ino_t)ino);
}

static void bridge_getxattr(fuse3_req_t req, fuse3_ino_t ino,
			    const char *name, size_t size)
{
	struct fuse2_session *session = session_from_req(req);

	if (session != NULL)
		fuse2_foundation_bridge_getxattr(session->ops.getxattr,
						 (fuse2_req_t)req,
						 (fuse2_ino_t)ino, name, size);
}

static void bridge_listxattr(fuse3_req_t req, fuse3_ino_t ino, size_t size)
{
	struct fuse2_session *session = session_from_req(req);

	if (session != NULL)
		fuse2_foundation_bridge_listxattr(session->ops.listxattr,
						  (fuse2_req_t)req,
						  (fuse2_ino_t)ino, size);
}

static void bridge_create(fuse3_req_t req, fuse3_ino_t parent,
			  const char *name, mode_t mode,
			  struct fuse3_file_info *fi)
{
	struct fuse2_session *session = session_from_req(req);
	struct fuse2_file_info legacy;

	if (session == NULL)
		return;
	file_info_3_to_2(fi, &legacy);
	fuse2_foundation_bridge_create(session->ops.create, (fuse2_req_t)req,
				       (fuse2_ino_t)parent, name, mode,
				       &legacy);
	file_info_2_to_3(&legacy, fi);
}

static void configure_backend_ops(struct fuse2_session *session,
				  struct fuse3_lowlevel_ops *ops)
{
	memset(ops, 0, sizeof(*ops));
	if (session->ops.init != NULL)
		ops->init = bridge_init;
	if (session->ops.destroy != NULL)
		ops->destroy = bridge_destroy;
	if (session->ops.lookup != NULL)
		ops->lookup = bridge_lookup;
	if (session->ops.forget != NULL)
		ops->forget = bridge_forget;
	if (session->ops.getattr != NULL)
		ops->getattr = bridge_getattr;
	if (session->ops.setattr != NULL)
		ops->setattr = bridge_setattr;
	if (session->ops.readlink != NULL)
		ops->readlink = bridge_readlink;
	if (session->ops.open != NULL)
		ops->open = bridge_open;
	if (session->ops.read != NULL)
		ops->read = bridge_read;
	if (session->ops.write != NULL)
		ops->write = bridge_write;
	if (session->ops.release != NULL)
		ops->release = bridge_release;
	if (session->ops.opendir != NULL)
		ops->opendir = bridge_opendir;
	if (session->ops.readdir != NULL)
		ops->readdir = bridge_readdir;
	if (session->ops.releasedir != NULL)
		ops->releasedir = bridge_releasedir;
	if (session->ops.statfs != NULL)
		ops->statfs = bridge_statfs;
	if (session->ops.getxattr != NULL)
		ops->getxattr = bridge_getxattr;
	if (session->ops.listxattr != NULL)
		ops->listxattr = bridge_listxattr;
	if (session->ops.create != NULL)
		ops->create = bridge_create;
}

int fuse2_native_opt_parse(struct fuse2_args *args, void *data,
			   const struct fuse2_opt opts[], fuse2_opt_proc_t proc)
{
	const struct fuse3_api *api = require_api();

	if (api == NULL)
		return -1;
	return api->opt_parse((struct fuse3_args *)args, data,
			      (const struct fuse3_opt *)opts,
			      (fuse3_opt_proc_t)proc);
}

int fuse2_native_opt_add_arg(struct fuse2_args *args, const char *arg)
{
	const struct fuse3_api *api = require_api();

	return api != NULL ?
		api->opt_add_arg((struct fuse3_args *)args, arg) : -1;
}

void fuse2_native_opt_free_args(struct fuse2_args *args)
{
	const struct fuse3_api *api = require_api();

	if (api != NULL)
		api->opt_free_args((struct fuse3_args *)args);
}

int fuse2_native_parse_cmdline(struct fuse2_args *args, char **mountpoint,
			       int *multithreaded, int *foreground)
{
	const struct fuse3_api *api = require_api();
	struct fuse3_cmdline_opts options;
	int result;

	if (api == NULL)
		return -1;
	memset(&options, 0, sizeof(options));
	result = api->parse_cmdline((struct fuse3_args *)args, &options);
	if (result != 0)
		return -1;
	if (multithreaded != NULL)
		*multithreaded = options.singlethread == 0;
	if (foreground != NULL)
		*foreground = options.foreground;
	if (mountpoint != NULL)
		*mountpoint = options.mountpoint;
	else
		free(options.mountpoint);
	return 0;
}

int fuse2_native_daemonize(int foreground)
{
	const struct fuse3_api *api = require_api();

	return api != NULL ? api->daemonize(foreground) : -1;
}

struct fuse2_chan *fuse2_native_mount(const char *mountpoint,
				      struct fuse2_args *args)
{
	struct fuse2_chan *chan;

	(void)args;
	if (mountpoint == NULL) {
		errno = EINVAL;
		return NULL;
	}
	if (require_api() == NULL)
		return NULL;
	chan = calloc(1, sizeof(*chan));
	if (chan == NULL)
		return NULL;
	chan->mountpoint = duplicate_string(mountpoint);
	if (chan->mountpoint == NULL) {
		free(chan);
		return NULL;
	}
	chan->magic = CHANNEL_MAGIC;
	fuse2_compat_log("queued legacy mount for %s", mountpoint);
	return chan;
}

struct fuse2_session *fuse2_native_lowlevel_new(
	struct fuse2_args *args, const struct fuse2_lowlevel_ops *ops,
	size_t op_size, void *userdata)
{
	const struct fuse3_api *api = require_api();
	struct fuse2_session *session;
	struct fuse3_lowlevel_ops backend_ops;
	size_t copy_size;

	if (api == NULL || args == NULL || ops == NULL) {
		errno = EINVAL;
		return NULL;
	}
	session = calloc(1, sizeof(*session));
	if (session == NULL)
		return NULL;
	session->magic = SESSION_MAGIC;
	session->userdata = userdata;
	copy_size = op_size < sizeof(session->ops) ? op_size : sizeof(session->ops);
	memcpy(&session->ops, ops, copy_size);
	configure_backend_ops(session, &backend_ops);
	session->backend = api->session_new((struct fuse3_args *)args,
					    &backend_ops,
					    sizeof(backend_ops), session);
	if (session->backend == NULL) {
		session->magic = 0;
		free(session);
		return NULL;
	}
	fuse2_compat_log("created translated low-level session");
	return session;
}

void fuse2_native_session_add_chan(struct fuse2_session *session,
				   struct fuse2_chan *chan)
{
	const struct fuse3_api *api = require_api();

	if (api == NULL || !valid_session(session) || !valid_chan(chan))
		return;
	if (api->session_mount(session->backend, chan->mountpoint) != 0) {
		session->mount_error = errno != 0 ? errno : EIO;
		fuse2_compat_log("mount failed for %s: %s", chan->mountpoint,
				 strerror(session->mount_error));
		return;
	}
	chan->mounted = true;
	chan->session = session;
	session->chan = chan;
	fuse2_compat_log("mounted FUSE3 session at %s", chan->mountpoint);
}

void fuse2_native_session_remove_chan(struct fuse2_chan *chan)
{
	const struct fuse3_api *api = require_api();
	struct fuse2_session *session;

	if (api == NULL || !valid_chan(chan))
		return;
	session = chan->session;
	if (chan->mounted && valid_session(session)) {
		api->session_unmount(session->backend);
		chan->mounted = false;
		fuse2_compat_log("unmounted FUSE3 session from %s", chan->mountpoint);
	}
	if (valid_session(session) && session->chan == chan)
		session->chan = NULL;
	chan->session = NULL;
}

int fuse2_native_session_loop(struct fuse2_session *session)
{
	const struct fuse3_api *api = require_api();

	if (api == NULL || !valid_session(session))
		return -1;
	if (session->mount_error != 0)
		return -session->mount_error;
	return api->session_loop(session->backend);
}

void fuse2_native_session_destroy(struct fuse2_session *session)
{
	const struct fuse3_api *api = require_api();

	if (api == NULL || !valid_session(session))
		return;
	if (session->chan != NULL && session->chan->mounted) {
		api->session_unmount(session->backend);
		session->chan->mounted = false;
	}
	if (session->chan != NULL)
		session->chan->session = NULL;
	api->session_destroy(session->backend);
	session->backend = NULL;
	session->magic = 0;
	free(session);
}

void fuse2_native_unmount(const char *mountpoint, struct fuse2_chan *chan)
{
	(void)mountpoint;
	if (!valid_chan(chan))
		return;
	if (chan->mounted || chan->session != NULL)
		fuse2_native_session_remove_chan(chan);
	chan->magic = 0;
	free(chan->mountpoint);
	free(chan);
}

int fuse2_native_set_signal_handlers(struct fuse2_session *session)
{
	const struct fuse3_api *api = require_api();

	return api != NULL && valid_session(session) ?
		api->set_signal_handlers(session->backend) : -1;
}

void fuse2_native_remove_signal_handlers(struct fuse2_session *session)
{
	const struct fuse3_api *api = require_api();

	if (api != NULL && valid_session(session))
		api->remove_signal_handlers(session->backend);
}

void fuse2_native_session_exit(struct fuse2_session *session)
{
	const struct fuse3_api *api = require_api();

	if (api != NULL && api->session_exit != NULL && valid_session(session))
		api->session_exit(session->backend);
}

int fuse2_native_session_exited(struct fuse2_session *session)
{
	const struct fuse3_api *api = require_api();

	return api != NULL && api->session_exited != NULL && valid_session(session) ?
		api->session_exited(session->backend) : 0;
}

void fuse2_native_session_reset(struct fuse2_session *session)
{
	const struct fuse3_api *api = require_api();

	if (api != NULL && api->session_reset != NULL && valid_session(session))
		api->session_reset(session->backend);
}

void *fuse2_native_req_userdata(fuse2_req_t req)
{
	struct fuse2_session *session = session_from_req((fuse3_req_t)req);

	return session != NULL ? session->userdata : NULL;
}

int fuse2_native_reply_err(fuse2_req_t req, int err)
{
	const struct fuse3_api *api = require_api();

	return api != NULL ? api->reply_err((fuse3_req_t)req, err) : -ENOSYS;
}

void fuse2_native_reply_none(fuse2_req_t req)
{
	const struct fuse3_api *api = require_api();

	if (api != NULL)
		api->reply_none((fuse3_req_t)req);
}

int fuse2_native_reply_entry(fuse2_req_t req,
			     const struct fuse2_entry_param *entry)
{
	const struct fuse3_api *api = require_api();
	struct fuse3_entry_param backend_entry;

	if (api == NULL || entry == NULL)
		return -ENOSYS;
	entry_2_to_3(entry, &backend_entry);
	return api->reply_entry((fuse3_req_t)req, &backend_entry);
}

int fuse2_native_reply_attr(fuse2_req_t req, const struct stat *attr,
			    double attr_timeout)
{
	const struct fuse3_api *api = require_api();

	return api != NULL ?
		api->reply_attr((fuse3_req_t)req, attr, attr_timeout) : -ENOSYS;
}

int fuse2_native_reply_open(fuse2_req_t req,
			    const struct fuse2_file_info *fi)
{
	const struct fuse3_api *api = require_api();
	struct fuse3_file_info backend_fi;

	if (api == NULL || fi == NULL)
		return -ENOSYS;
	memset(&backend_fi, 0, sizeof(backend_fi));
	file_info_2_to_3(fi, &backend_fi);
	return api->reply_open((fuse3_req_t)req, &backend_fi);
}

int fuse2_native_reply_buf(fuse2_req_t req, const char *buf, size_t size)
{
	const struct fuse3_api *api = require_api();

	return api != NULL ? api->reply_buf((fuse3_req_t)req, buf, size) :
		-ENOSYS;
}

int fuse2_native_reply_readlink(fuse2_req_t req, const char *link)
{
	const struct fuse3_api *api = require_api();

	return api != NULL ? api->reply_readlink((fuse3_req_t)req, link) :
		-ENOSYS;
}

int fuse2_native_reply_xattr(fuse2_req_t req, size_t count)
{
	const struct fuse3_api *api = require_api();

	return api != NULL ? api->reply_xattr((fuse3_req_t)req, count) :
		-ENOSYS;
}

int fuse2_native_reply_write(fuse2_req_t req, size_t count)
{
	const struct fuse3_api *api = require_api();

	return api != NULL ? api->reply_write((fuse3_req_t)req, count) :
		-ENOSYS;
}

int fuse2_native_reply_statfs(fuse2_req_t req,
			      const struct statvfs *statfsbuf)
{
	const struct fuse3_api *api = require_api();

	return api != NULL ? api->reply_statfs((fuse3_req_t)req, statfsbuf) :
		-ENOSYS;
}

int fuse2_native_reply_create(fuse2_req_t req,
			      const struct fuse2_entry_param *entry,
			      const struct fuse2_file_info *fi)
{
	const struct fuse3_api *api = require_api();
	struct fuse3_entry_param backend_entry;
	struct fuse3_file_info backend_fi;

	if (api == NULL || entry == NULL || fi == NULL)
		return -ENOSYS;
	entry_2_to_3(entry, &backend_entry);
	memset(&backend_fi, 0, sizeof(backend_fi));
	file_info_2_to_3(fi, &backend_fi);
	return api->reply_create((fuse3_req_t)req, &backend_entry, &backend_fi);
}

int fuse2_native_reply_lock(fuse2_req_t req, const struct flock *lock)
{
	const struct fuse3_api *api = require_api();

	return api != NULL ? api->reply_lock((fuse3_req_t)req, lock) : -ENOSYS;
}

int fuse2_native_reply_bmap(fuse2_req_t req, uint64_t idx)
{
	const struct fuse3_api *api = require_api();

	return api != NULL ? api->reply_bmap((fuse3_req_t)req, idx) : -ENOSYS;
}

size_t fuse2_native_add_direntry(fuse2_req_t req, char *buf, size_t bufsize,
				 const char *name,
				 const struct stat *statbuf, off_t off)
{
	const struct fuse3_api *api = require_api();

	return api != NULL ? api->add_direntry((fuse3_req_t)req, buf, bufsize,
					       name, statbuf, off) : 0;
}
