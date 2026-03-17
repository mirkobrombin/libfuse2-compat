/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "fuse3_loader.h"

#include <dlfcn.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static struct fuse3_api api;
static pthread_once_t api_once = PTHREAD_ONCE_INIT;
static char api_error[256];

static bool logging_enabled(void)
{
	const char *value = getenv("LIBFUSE2_COMPAT_LOG");

	return value != NULL && value[0] != '\0' && strcmp(value, "0") != 0;
}

void fuse2_compat_log(const char *format, ...)
{
	va_list args;

	if (!logging_enabled())
		return;

	fputs("libfuse2-compat: ", stderr);
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	fputc('\n', stderr);
}

static void set_error(const char *message)
{
	(void)snprintf(api_error, sizeof(api_error), "%s", message);
}

static bool load_symbol(void *destination, size_t destination_size,
			const char *name, bool required)
{
	void *symbol;
	const char *error;

	dlerror();
	symbol = dlsym(api.handle, name);
	error = dlerror();
	if (error != NULL) {
		if (required) {
			(void)snprintf(api_error, sizeof(api_error),
				       "missing FUSE3 symbol %s: %s", name, error);
		}
		return !required;
	}

	if (destination_size != sizeof(symbol)) {
		set_error("function pointer size is unsupported on this platform");
		return false;
	}
	memcpy(destination, &symbol, sizeof(symbol));
	return true;
}

#define LOAD_REQUIRED(field, symbol) \
	do { \
		if (!load_symbol(&api.field, sizeof(api.field), symbol, true)) \
			return; \
	} while (0)

#define LOAD_OPTIONAL(field, symbol) \
	(void)load_symbol(&api.field, sizeof(api.field), symbol, false)

static void load_api(void)
{
	const char *override = getenv("LIBFUSE2_COMPAT_BACKEND");
	const char *candidates[] = { "libfuse3.so.4", "libfuse3.so.3", NULL };
	const char **candidate;

	memset(&api, 0, sizeof(api));
	api_error[0] = '\0';

	if (override != NULL && override[0] != '\0') {
		api.handle = dlopen(override, RTLD_NOW | RTLD_LOCAL);
		api.backend_name = override;
	} else {
		for (candidate = candidates; *candidate != NULL; candidate++) {
			api.handle = dlopen(*candidate, RTLD_NOW | RTLD_LOCAL);
			if (api.handle != NULL) {
				api.backend_name = *candidate;
				break;
			}
		}
	}

	if (api.handle == NULL) {
		const char *error = dlerror();
		(void)snprintf(api_error, sizeof(api_error),
			       "unable to load libfuse3: %s",
			       error != NULL ? error : "unknown loader error");
		return;
	}

	LOAD_REQUIRED(opt_parse, "fuse_opt_parse");
	LOAD_REQUIRED(opt_add_arg, "fuse_opt_add_arg");
	LOAD_REQUIRED(opt_free_args, "fuse_opt_free_args");
	LOAD_REQUIRED(parse_cmdline, "fuse_parse_cmdline");
	LOAD_REQUIRED(session_new, "fuse_session_new");
	LOAD_REQUIRED(session_mount, "fuse_session_mount");
	LOAD_REQUIRED(session_unmount, "fuse_session_unmount");
	LOAD_REQUIRED(session_loop, "fuse_session_loop");
	LOAD_REQUIRED(session_destroy, "fuse_session_destroy");
	LOAD_REQUIRED(set_signal_handlers, "fuse_set_signal_handlers");
	LOAD_REQUIRED(remove_signal_handlers, "fuse_remove_signal_handlers");
	LOAD_REQUIRED(daemonize, "fuse_daemonize");
	LOAD_REQUIRED(req_userdata, "fuse_req_userdata");
	LOAD_REQUIRED(reply_err, "fuse_reply_err");
	LOAD_REQUIRED(reply_none, "fuse_reply_none");
	LOAD_REQUIRED(reply_entry, "fuse_reply_entry");
	LOAD_REQUIRED(reply_attr, "fuse_reply_attr");
	LOAD_REQUIRED(reply_open, "fuse_reply_open");
	LOAD_REQUIRED(reply_buf, "fuse_reply_buf");
	LOAD_REQUIRED(reply_readlink, "fuse_reply_readlink");
	LOAD_REQUIRED(reply_xattr, "fuse_reply_xattr");
	LOAD_REQUIRED(reply_write, "fuse_reply_write");
	LOAD_REQUIRED(reply_statfs, "fuse_reply_statfs");
	LOAD_REQUIRED(reply_create, "fuse_reply_create");
	LOAD_REQUIRED(reply_lock, "fuse_reply_lock");
	LOAD_REQUIRED(reply_bmap, "fuse_reply_bmap");
	LOAD_REQUIRED(add_direntry, "fuse_add_direntry");

	LOAD_OPTIONAL(session_exit, "fuse_session_exit");
	LOAD_OPTIONAL(session_exited, "fuse_session_exited");
	LOAD_OPTIONAL(session_reset, "fuse_session_reset");

	fuse2_compat_log("using backend %s", api.backend_name);
}

const struct fuse3_api *fuse3_api_get(void)
{
	(void)pthread_once(&api_once, load_api);
	return api.handle != NULL && api_error[0] == '\0' ? &api : NULL;
}

const char *fuse3_api_error(void)
{
	(void)pthread_once(&api_once, load_api);
	return api_error[0] != '\0' ? api_error : NULL;
}
