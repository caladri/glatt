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

struct fs_file {
	char fsf_path[64];
	struct fs *fsf_fs;
	fs_file_context_t fsf_context;
};

static int fs_file_ipc_read_handler(struct fs_file *, const struct ipc_header *, void **);
static int fs_file_ipc_close_handler(struct fs_file *, const struct ipc_header *, void **);
#ifdef EXEC
static int fs_file_ipc_exec_handler(struct fs_file *, const struct ipc_header *, void **);
#endif
static int fs_file_ipc_service_start(struct fs_file *, ipc_port_t *);

int
fs_ipc_open_file_handler(struct fs *fs, const struct ipc_header *reqh, void **pagep)
{
	struct fs_open_file_request *req;
	struct ipc_header ipch;
	fs_file_context_t fsfc;
	struct fs_file *fsf;
	ipc_port_t port;
	int error;

	if (pagep == NULL)
		return (ERROR_INVALID);
	req = *pagep;

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
fs_file_ipc_handler(void *softc, struct ipc_header *ipch, void **pagep)
{
	struct fs_file *fsf = softc;

	switch (ipch->ipchdr_msg) {
	case FS_FILE_MSG_READ:
		return (fs_file_ipc_read_handler(fsf, ipch, pagep));
	case FS_FILE_MSG_CLOSE:
		return (fs_file_ipc_close_handler(fsf, ipch, pagep));
#ifdef EXEC
	case FS_FILE_MSG_EXEC:
		return (fs_file_ipc_exec_handler(fsf, ipch, pagep));
#endif
	default:
		/* Don't respond to nonsense.  */
		return (ERROR_INVALID);
	}
}

static int
fs_file_ipc_read_handler(struct fs_file *fsf, const struct ipc_header *reqh, void **pagep)
{
	fs_file_context_t fsfc = fsf->fsf_context;
	struct fs *fs = fsf->fsf_fs;
	struct fs_file_read_request *req;
	struct ipc_header ipch;
	size_t length;
	int error;

	if (pagep == NULL)
		return (ERROR_INVALID);
	req = *pagep;

	if (req->length == 0)
		return (ERROR_INVALID);

	/*
	 * Note that we reuse p as a buffer.
	 */
	length = req->length > PAGE_SIZE ? PAGE_SIZE : req->length;
	error = fs->fs_ops->fs_file_read(fs->fs_context, fsfc, *pagep, req->offset, &length);
	if (error != 0) {
		ipch = IPC_HEADER_ERROR(reqh, error);

		error = ipc_port_send_data(&ipch, NULL, 0);
	} else {
		ipch = IPC_HEADER_REPLY(reqh);
		ipch.ipchdr_param = length;

		if (length == 0) {
			error = ipc_port_send_data(&ipch, NULL, 0);
		} else {
			error = ipc_port_send(&ipch, *pagep);
			/* XXX Does ipc_port_send always consume the page [on error]?  */
			*pagep = NULL;
		}
	}

	if (error != 0)
		return (error);
	return (0);
}

static int
fs_file_ipc_close_handler(struct fs_file *fsf, const struct ipc_header *reqh, void **pagep)
{
	fs_file_context_t fsfc = fsf->fsf_context;
	struct fs *fs = fsf->fsf_fs;
	struct ipc_header ipch;
	int error;

	if (pagep != NULL)
		return (ERROR_INVALID);

	/*
	 * XXX
	 * Need to reference-count the service and only do this close once all
	 * users are done, and drop the receive right, and have dropping the
	 * receive right lead to garbage-collecting the port.
	 *
	 * At that point, do we even need a close(), or can the caller just drop
	 * the right to this port?  But then they need to refcount rights and be
	 * sure they only have this file open once.
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
fs_file_ipc_exec_handler(struct fs_file *fsf, const struct ipc_header *reqh, void **pagep)
{
	fs_file_context_t fsfc = fsf->fsf_context;
	struct fs *fs = fsf->fsf_fs;
	struct ipc_header ipch;
	ipc_port_t child;
	int error;

	if (pagep != NULL)
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
