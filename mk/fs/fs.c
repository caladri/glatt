#include <core/types.h>
#include <core/error.h>
#ifdef EXEC
#include <core/exec.h>
#endif
#include <core/malloc.h>
#include <core/spinlock.h>
#include <core/startup.h>
#include <core/string.h>
#include <core/task.h>
#include <core/thread.h>
#include <core/console.h>
#include <ipc/ipc.h>
#include <ipc/port.h>
#include <ipc/service.h>
#include <ipc/task.h>
#include <fs/fs.h>
#include <fs/fs_ipc.h>
#include <fs/fs_ops.h>
#include <vm/vm.h>
#include <vm/vm_alloc.h>

static STAILQ_HEAD(, struct fs) fs_list = STAILQ_HEAD_INITIALIZER(fs_list);

#ifdef EXEC
static int fs_exec(struct fs *, const char *);
#endif

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

	error = ipc_service(name, IPC_PORT_UNKNOWN, IPC_PORT_FLAG_PUBLIC | IPC_PORT_FLAG_NEW,
			    fs_ipc_handler, fs);
	if (error != 0) {
		free(fs);
		return (error);
	}

	STAILQ_INSERT_TAIL(&fs_list, fs, fs_link);

	return (0);
}

#ifdef EXEC
static int
fs_exec(struct fs *fs, const char *path)
{
	fs_file_context_t fsfc;
	int error, error2;

	error = fs->fs_ops->fs_file_open(fs->fs_context, path, &fsfc);
	if (error != 0)
		return (error);

	error = exec_task(IPC_PORT_UNKNOWN, NULL, path, fs->fs_ops->fs_file_read, fs->fs_context, fsfc);

	error2 = fs->fs_ops->fs_file_close(fs->fs_context, fsfc);
	if (error2 != 0)
		panic("%s: file close failed: %m", __func__, error2);

	if (error != 0)
		return (error);

	return (0);
}

#define	FS_BOOTSTRAP_PATH	"/sbin/bootstrap"
static void
fs_autorun(void *arg)
{
	struct fs *fs;
	unsigned cnt;
	int error;

	(void)arg;

	cnt = 0;
	STAILQ_FOREACH(fs, &fs_list, fs_link) {
		error = fs_exec(fs, FS_BOOTSTRAP_PATH);
		if (error != 0)
			continue;
		cnt++;
	}
	if (cnt == 0)
		printf("%s: No bootstrap found.\n", __func__);
	else if (cnt != 1)
		printf("%s: Multiple bootstraps found.\n", __func__);
}
STARTUP_ITEM(fs_autorun, STARTUP_SERVERS, STARTUP_SECOND, fs_autorun, NULL);
#endif
