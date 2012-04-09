#include <core/types.h>
#include <core/error.h>
#ifdef EXEC
#include <core/exec.h>
#endif
#include <core/malloc.h>
#include <core/spinlock.h>
#include <core/startup.h>
#include <core/string.h>
#include <io/console/console.h>
#include <ipc/ipc.h>
#include <ipc/port.h>
#include <ipc/service.h>
#include <fs/fs.h>
#include <fs/fs_ops.h>

struct fs {
	struct fs_ops *fs_ops;
	fs_context_t fs_context;
	STAILQ_ENTRY(struct fs) fs_link;
};

static STAILQ_HEAD(, struct fs) fs_list = STAILQ_HEAD_INITIALIZER(fs_list);

static ipc_service_t fs_ipc_handler;

int
fs_register(const char *name, struct fs_ops *fso, fs_context_t fsc)
{
	struct fs *fs;
	int error;

	fs = malloc(sizeof *fs);
	if (fs == NULL)
		return (ERROR_EXHAUSTED);

	fs->fs_ops = fso;
	fs->fs_context = fsc;

	error = ipc_service(name, IPC_PORT_UNKNOWN, IPC_PORT_FLAG_PUBLIC,
			    fs_ipc_handler, fs);
	if (error != 0) {
		free(fs);
		return (error);
	}

	STAILQ_INSERT_TAIL(&fs_list, fs, fs_link);

	return (0);
}

static int
fs_ipc_handler(void *softc, struct ipc_header *ipch, void *p)
{
	struct fs *fs = softc;

	(void)fs;

	switch (ipch->ipchdr_msg) {
	default:
		/* Don't respond to nonsense.  */
		return (ERROR_INVALID);
	}
}

#ifdef EXEC
static void
fs_autorun(void *arg)
{
	fs_file_context_t fsfc;
	struct fs *fs;
	int error;

	(void)arg;

	STAILQ_FOREACH(fs, &fs_list, fs_link) {
		/*
		 * XXX
		 * Need to implement directory operations Real Soon Now,
		 * then go back to launching all the things, like UFS used
		 * to.
		 */
		error = fs->fs_ops->fs_file_open(fs->fs_context, "/mu/servers/mu-shell", &fsfc);
		if (error != 0) {
			kcprintf("%s: file open failed: %m\n", __func__, error);
			continue;
		}

		error = exec_task("/mu/servers/mu-shell", fs->fs_ops->fs_file_read, fs->fs_context, fsfc);
		if (error != 0) {
			kcprintf("%s: exec_task failed: %m\n", __func__, error);
			continue;
		}

		error = fs->fs_ops->fs_file_close(fs->fs_context, fsfc);
		if (error != 0) {
			kcprintf("%s: file close failed: %m\n", __func__, error);
			continue;
		}
	}
}
STARTUP_ITEM(fs_autorun, STARTUP_SERVERS, STARTUP_SECOND, fs_autorun, NULL);
#endif
