#include <core/types.h>
#include <core/error.h>
#ifdef EXEC
#include <core/exec.h>
#endif
#include <core/malloc.h>
#include <core/spinlock.h>
#include <core/startup.h>
#include <core/string.h>
#include <core/console.h>
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
static int fs_file_ipc_read_handler(struct fs_file *, const struct ipc_header *, void *);
static int fs_file_ipc_close_handler(struct fs_file *, const struct ipc_header *, void *);
#ifdef EXEC
static int fs_file_ipc_exec_handler(struct fs_file *, const struct ipc_header *, void *);
#endif
static int fs_file_ipc_service_start(struct fs_file *, ipc_port_t *);

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
	struct fs_open_file_request *req;
	struct ipc_header ipch;
	fs_file_context_t fsfc;
	struct fs_file *fsf;
	ipc_port_t port;
	int error;

	if (p == NULL)
		return (ERROR_INVALID);
	req = p;

	error = fs->fs_ops->fs_file_open(fs->fs_context, req->path, &fsfc);
	if (error != 0) {
		ipch = IPC_HEADER_ERROR(reqh, error);

		error = ipc_port_send_data(&ipch, NULL, 0);
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
	 * [... Previous comments resolved ...]
	 *
	 * Add to that that we want this service to be refcounted and to go away
	 * when all its send rights disappear, and this exposes quite a lot of
	 * work that still needs to be done.
	 *
	 * NB: All ports will have to be refcounted in that way.
	 */

	error = fs_file_ipc_service_start(fsf, &port);
	if (error != 0) {
		ipch = IPC_HEADER_ERROR(reqh, error);

		free(fsf);

		error = fs->fs_ops->fs_file_close(fs->fs_context, fsfc);
		if (error != 0)
			panic("%s: file close failed: %m", __func__, error);
	} else {
		error = ipc_port_right_send(reqh->ipchdr_src, port, IPC_PORT_RIGHT_SEND);
		if (error != 0) {
			printf("%s: ipc_port_right_send failed: %m\n", __func__, error);
			/* XXX Shut down the service?  */
			return (error);
		}

		if (ipc_port_right_drop(port, IPC_PORT_RIGHT_RECEIVE) != 0)
			panic("%s: ipc_port_right_drop failed.", __func__);

		ipch = IPC_HEADER_REPLY(reqh);
		ipch.ipchdr_param = port;
	}

	error = ipc_port_send_data(&ipch, NULL, 0);
	if (error != 0) {
		printf("%s: ipc_port_send failed: %m\n", __func__, error);
		return (error);
	}

	return (0);
}

static int
fs_file_ipc_handler(void *softc, struct ipc_header *ipch, void *p)
{
	struct fs_file *fsf = softc;

	switch (ipch->ipchdr_msg) {
	case FS_FILE_MSG_READ:
		return (fs_file_ipc_read_handler(fsf, ipch, p));
	case FS_FILE_MSG_CLOSE:
		return (fs_file_ipc_close_handler(fsf, ipch, p));
#ifdef EXEC
	case FS_FILE_MSG_EXEC:
		return (fs_file_ipc_exec_handler(fsf, ipch, p));
#endif
	default:
		/* Don't respond to nonsense.  */
		return (ERROR_INVALID);
	}
}

static int
fs_file_ipc_read_handler(struct fs_file *fsf, const struct ipc_header *reqh, void *p)
{
	fs_file_context_t fsfc = fsf->fsf_context;
	struct fs *fs = fsf->fsf_fs;
	struct fs_file_read_request *req;
	struct ipc_header ipch;
	size_t length;
	int error;

	if (p == NULL)
		return (ERROR_INVALID);
	req = p;

	if (req->length == 0)
		return (ERROR_INVALID);

	/*
	 * Note that we reuse p as a buffer.
	 */
	length = req->length > PAGE_SIZE ? PAGE_SIZE : req->length;
	error = fs->fs_ops->fs_file_read(fs->fs_context, fsfc, p, req->offset, &length);
	if (error != 0) {
		ipch = IPC_HEADER_ERROR(reqh, error);

		error = ipc_port_send_data(&ipch, NULL, 0);
	} else {
		ipch = IPC_HEADER_REPLY(reqh);
		ipch.ipchdr_param = length;

		/*
		 * XXX
		 * We don't just send p directly because ipc_service is still
		 * messy about whether pages are freed or not, to say nothing
		 * of error handling.  Better to copy for now.
		 */
		error = ipc_port_send_data(&ipch, p, length);
	}

	if (error != 0)
		return (error);
	return (0);
}

static int
fs_file_ipc_close_handler(struct fs_file *fsf, const struct ipc_header *reqh, void *p)
{
	fs_file_context_t fsfc = fsf->fsf_context;
	struct fs *fs = fsf->fsf_fs;
	struct ipc_header ipch;
	int error;

	if (p != NULL)
		return (ERROR_INVALID);

	/*
	 * XXX
	 * Need to reference-count the service and only do this close once all
	 * users are done, and drop the receive right, and have dropping the
	 * receive right lead to garbage-collecting the port.
	 */

	error = fs->fs_ops->fs_file_close(fs->fs_context, fsfc);
	if (error != 0) {
		ipch = IPC_HEADER_ERROR(reqh, error);
	} else {
		ipch = IPC_HEADER_REPLY(reqh);
	}

	error = ipc_port_send_data(&ipch, NULL, 0);
	if (error != 0)
		return (error);

	/*
	 * XXX
	 * Check outstanding send rights and then ipc_port_right_drop.
	 */

	return (0);
}

#ifdef EXEC
static int
fs_file_ipc_exec_handler(struct fs_file *fsf, const struct ipc_header *reqh, void *p)
{
	fs_file_context_t fsfc = fsf->fsf_context;
	struct fs *fs = fsf->fsf_fs;
	struct ipc_header ipch;
	ipc_port_t child;
	int error;

	if (p != NULL)
		return (ERROR_INVALID);

	error = exec_task(reqh->ipchdr_src, &child, fsf->fsf_path, fs->fs_ops->fs_file_read, fs->fs_context, fsfc);
	if (error != 0) {
		ipch = IPC_HEADER_ERROR(reqh, error);
	} else {
		ipch = IPC_HEADER_REPLY(reqh);
		ipch.ipchdr_param = child;
	}

	error = ipc_port_send_data(&ipch, NULL, 0);
	if (error != 0)
		return (error);
	return (0);
}
#endif

static int
fs_file_ipc_service_start(struct fs_file *fsf, ipc_port_t *portp)
{
	ipc_port_t port;
	int error;

	/*
	 * XXX
	 * Push all of this into one routine so then we can make the error reply
	 * easier?  fs_file_service_start(&port, fsfc...);
	 */
	error = ipc_port_allocate(&port, IPC_PORT_FLAG_DEFAULT);
	if (error != 0)
		return (error);

	error = ipc_service(fsf->fsf_path, port, IPC_PORT_FLAG_DEFAULT,
			    fs_file_ipc_handler, fsf);
	if (error != 0) {
		if (ipc_port_right_drop(port, IPC_PORT_RIGHT_RECEIVE) != 0)
			panic("%s: ipc_port_right_drop failed.", __func__);
		return (error);
	}

	*portp = port;

	return (0);
}

#ifdef EXEC
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
