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
#include <ipc/task.h>
#include <fs/fs.h>
#include <fs/fs_ops.h>

struct fs {
	struct fs_ops *fs_ops;
	fs_context_t fs_context;
	STAILQ_ENTRY(struct fs) fs_link;
};

struct fs_file {
	char fsf_path[64];
	struct fs *fsf_fs;
	fs_file_context_t fsf_context;
};

static STAILQ_HEAD(, struct fs) fs_list = STAILQ_HEAD_INITIALIZER(fs_list);

#ifdef EXEC
static int fs_exec(struct fs *, const char *);
#endif
static ipc_service_t fs_ipc_handler;
static int fs_ipc_open_file_handler(struct fs *, const struct ipc_header *, void *);
static ipc_service_t fs_file_ipc_handler;
#ifdef EXEC
static int fs_file_ipc_exec_handler(struct fs_file *, const struct ipc_header *, void *);
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

	error = ipc_service(name, true, IPC_PORT_UNKNOWN, IPC_PORT_FLAG_PUBLIC, NULL,
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

	error = exec_task(path, fs->fs_ops->fs_file_read, fs->fs_context, fsfc);

	error2 = fs->fs_ops->fs_file_close(fs->fs_context, fsfc);
	if (error2 != 0)
		panic("%s: file close failed: %m", __func__, error2);

	if (error != 0)
		return (error);

	return (0);
}
#endif

static int
fs_ipc_handler(void *softc, struct ipc_header *ipch, void *p)
{
	struct fs *fs = softc;

	switch (ipch->ipchdr_msg) {
	case FS_MSG_OPEN_FILE:
		return (fs_ipc_open_file_handler(fs, ipch, p));
	default:
		/* Don't respond to nonsense.  */
		return (ERROR_INVALID);
	}
}

static int
fs_ipc_open_file_handler(struct fs *fs, const struct ipc_header *reqh, void *p)
{
	const struct ipc_service_context *svc;
	struct fs_open_file_request *req;
	struct fs_open_file_response resp;
	struct ipc_error_record err;
	struct ipc_header ipch;
	fs_file_context_t fsfc;
	struct fs_file *fsf;
	int error;

	if (reqh->ipchdr_recsize != sizeof *req || reqh->ipchdr_reccnt != 1 || p == NULL)
		return (ERROR_INVALID);
	req = p;

	error = fs->fs_ops->fs_file_open(fs->fs_context, req->path, &fsfc);
	if (error != 0) {
		err.error = error;

		ipch = IPC_HEADER_ERROR(reqh);
		ipch.ipchdr_recsize = sizeof err;
		ipch.ipchdr_reccnt = 1;

		error = ipc_port_send_data(&ipch, &err, sizeof err);
		if (error != 0)
			return (error);
		return (0);
	}

	fsf = malloc(sizeof *fsf);
	if (fsf == NULL)
		return (ERROR_EXHAUSTED);
	strlcpy(fsf->fsf_path, req->path, sizeof fsf->fsf_path);
	fsf->fsf_fs = fs;
	fsf->fsf_context = fsfc;

	/*
	 * XXX
	 * If this is PUBLIC then we can only open a file once, because the
	 * name is registered with NS.  Making it not public means we have a
	 * rights management issue.  Making it public but adding a flag to not
	 * register it with NS has confusing semantics, too.
	 *
	 * Add to that that we want this service to be refcounted and to go away
	 * when all its send rights disappear, and this exposes quite a lot of
	 * work that still needs to be done.
	 */
	error = ipc_service(req->path, false, IPC_PORT_UNKNOWN, IPC_PORT_FLAG_PUBLIC, &svc,
			    fs_file_ipc_handler, fsf);
	if (error != 0) {
		err.error = error;

		ipch = IPC_HEADER_ERROR(reqh);
		ipch.ipchdr_recsize = sizeof err;
		ipch.ipchdr_reccnt = 1;

		free(fsf);

		error = fs->fs_ops->fs_file_close(fs->fs_context, fsfc);
		if (error != 0)
			panic("%s: file close failed: %m", __func__, error);

		error = ipc_port_send_data(&ipch, &err, sizeof err);
		if (error != 0)
			return (error);
		return (0);
	}

	resp.file_port = ipc_service_port(svc);

	/*
	 * XXX
	 * See above about making the service PUBLIC.
	 */
#if 0
	error = ipc_port_send_right(resp.file_port, reqh->ipchdr_src, IPC_PORT_RIGHT_SEND);
	if (error != 0) {
		kcprintf("%s: ipc_port_send_right failed: %m\n", __func__, error);
		/* XXX Shut down the service?  */
		return (error);
	}
#endif

	ipch = IPC_HEADER_REPLY(reqh);
	ipch.ipchdr_recsize = sizeof resp;
	ipch.ipchdr_reccnt = 1;

	error = ipc_port_send_data(&ipch, &resp, sizeof resp);
	if (error != 0) {
		kcprintf("%s: ipc_port_send failed: %m\n", __func__, error);
		return (error);
	}

	return (0);
}

static int
fs_file_ipc_handler(void *softc, struct ipc_header *ipch, void *p)
{
	struct fs_file *fsf = softc;

	switch (ipch->ipchdr_msg) {
#ifdef EXEC
	case FS_FILE_MSG_EXEC:
		return (fs_file_ipc_exec_handler(fsf, ipch, p));
#endif
	default:
		/* Don't respond to nonsense.  */
		return (ERROR_INVALID);
	}
}

#ifdef EXEC
static int
fs_file_ipc_exec_handler(struct fs_file *fsf, const struct ipc_header *reqh, void *p)
{
	fs_file_context_t fsfc = fsf->fsf_context;
	struct fs *fs = fsf->fsf_fs;
	struct ipc_error_record err;
	struct ipc_header ipch;
	int error;

	if (reqh->ipchdr_recsize != 0 || reqh->ipchdr_reccnt != 0 || p != NULL)
		return (ERROR_INVALID);

	error = exec_task(fsf->fsf_path, fs->fs_ops->fs_file_read, fs->fs_context, fsfc);
	if (error != 0) {
		err.error = error;

		ipch = IPC_HEADER_ERROR(reqh);
		ipch.ipchdr_recsize = sizeof err;
		ipch.ipchdr_reccnt = 1;

		error = ipc_port_send_data(&ipch, &err, sizeof err);
	} else {
		ipch = IPC_HEADER_REPLY(reqh);

		error = ipc_port_send_data(&ipch, NULL, 0);
	}

	if (error != 0)
		return (error);
	return (0);
}
#endif

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
