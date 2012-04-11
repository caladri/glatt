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

static int fs_exec(struct fs *, const char *);
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
fs_exec(struct fs *fs, const char *path)
{
	fs_file_context_t fsfc;
	int error, error2;

	error = fs->fs_ops->fs_file_open(fs->fs_context, path, &fsfc);
	if (error != 0)
		return (error);

	error = exec_task(path, fs->fs_ops->fs_file_read, fs->fs_context, fsfc);

	error2 = fs->fs_ops->fs_file_close(fs->fs_context, fsfc);
	if (error2 != 0)
		panic("%s: file close failed: %m", __func__, error2);

	if (error != 0)
		return (error);

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
#define	FS_AUTORUN_DIR		"/mu/servers"
static char fs_autorun_path[1024];
static struct fs_directory_entry fs_autorun_entry;

static void
fs_autorun(void *arg)
{
	fs_directory_context_t fsdc;
	struct fs *fs;
	off_t offset;
	size_t cnt;
	int error;

	(void)arg;

	/*
	 * XXX
	 * This leaks files and directories.
	 */
	STAILQ_FOREACH(fs, &fs_list, fs_link) {
		if (fs->fs_ops->fs_directory_open == NULL) {
#ifdef VERBOSE
			kcprintf("%s: skipping filesystem without directory open method.\n", __func__);
#endif
			continue;
		}
		if (fs->fs_ops->fs_file_open == NULL) {
#ifdef VERBOSE
			kcprintf("%s: skipping filesystem without file open method.\n", __func__);
#endif
			continue;
		}

		error = fs->fs_ops->fs_directory_open(fs->fs_context, FS_AUTORUN_DIR, &fsdc);
		if (error != 0) {
			kcprintf("%s: directory open failed: %m\n", __func__, error);
			continue;
		}

		offset = 0;
		for (;;) {
			cnt = 1;
			error = fs->fs_ops->fs_directory_read(fs->fs_context, fsdc, &fs_autorun_entry, &offset, &cnt);
			if (error != 0) {
				kcprintf("%s: directory read failed: %m\n", __func__, error);
				break;
			}

			if (cnt == 0)
				break;

			if (cnt != 1)
				panic("%s: implausible number of directory entries: %zu", __func__, cnt);

			/*
			 * Skip files with leading dot.
			 */
			if (fs_autorun_entry.name[0] == '.')
				continue;

			snprintf(fs_autorun_path, sizeof fs_autorun_path, "%s/%s", FS_AUTORUN_DIR, fs_autorun_entry.name);

			error = fs_exec(fs, fs_autorun_path);
			if (error != 0) {
				kcprintf("%s: file exec failed: %m", __func__, error);
				continue;
			}
		}
	}
}
STARTUP_ITEM(fs_autorun, STARTUP_SERVERS, STARTUP_SECOND, fs_autorun, NULL);
#endif
