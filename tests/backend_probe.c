/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "fuse2_exports.h"

#include <stdio.h>
#include <stdlib.h>

int main(void)
{
	char *argv[] = { "backend-probe", NULL };
	struct fuse2_args args = { 1, argv, 0 };
	struct fuse2_chan *chan;

	chan = fuse_mount("/tmp/libfuse2-compat-probe", &args);
	if (chan == NULL) {
		fputs("system FUSE3 backend probe: failed\n", stderr);
		return EXIT_FAILURE;
	}
	fuse_unmount("/tmp/libfuse2-compat-probe", chan);
	puts("system FUSE3 backend probe: ok");
	return EXIT_SUCCESS;
}
