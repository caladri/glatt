#include <core/types.h>
#include <core/error.h>
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

struct fs_directory {
	char fsd_path[64];
	struct fs *fsd_fs;
	fs_directory_context_t fsd_context;
};

static int fs_directory_ipc_read_handler(struct fs_directory *, const struct ipc_header *, void *);
static int fs_directory_ipc_close_handler(struct fs_directory *, const struct ipc_header *, void *);
static int fs_directory_ipc_service_start(struct fs_directory *, ipc_port_t *);

int
fs_ipc_open_directory_handler(struct fs *fs, const struct ipc_header *reqh, void *p)
{
	struct fs_open_file_request *req;
	struct ipc_header ipch;
	fs_directory_context_t fsdc;
	struct fs_directory *fsd;
	ipc_port_t port;
	int error;

	if (p == NULL)
		return (ERROR_INVALID);
	req = p;

	error = fs->fs_ops->fs_directory_open(fs->fs_context, req->path, &fsdc);
	if (error != 0) {
		ipch = IPC_HEADER_ERROR(reqh, error);

		error = ipc_port_send_data(&ipch, NULL, 0);
		if (error != 0)
			return (error);
		return (0);
	}

	fsd = malloc(sizeof *fsd);
	if (fsd == NULL)
		return (ERROR_EXHAUSTED);
	strlcpy(fsd->fsd_path, req->path, sizeof fsd->fsd_path);
	fsd->fsd_fs = fs;
	fsd->fsd_context = fsdc;

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

	error = fs_directory_ipc_service_start(fsd, &port);
	if (error != 0) {
		ipch = IPC_HEADER_ERROR(reqh, error);

		free(fsd);

		error = fs->fs_ops->fs_directory_close(fs->fs_context, fsdc);
		if (error != 0)
			panic("%s: directory close failed: %m", __func__, error);
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
fs_directory_ipc_handler(void *softc, struct ipc_header *ipch, void *p)
{
	struct fs_directory *fsd = softc;

	switch (ipch->ipchdr_msg) {
	case FS_DIRECTORY_MSG_READ:
		return (fs_directory_ipc_read_handler(fsd, ipch, p));
	case FS_DIRECTORY_MSG_CLOSE:
		return (fs_directory_ipc_close_handler(fsd, ipch, p));
	default:
		/* Don't respond to nonsense.  */
		return (ERROR_INVALID);
	}
}

static int
fs_directory_ipc_read_handler(struct fs_directory *fsd, const struct ipc_header *reqh, void *p)
{
	fs_directory_context_t fsdc = fsd->fsd_context;
	struct fs *fs = fsd->fsd_fs;
	struct fs_directory_read_request *req;
	struct fs_directory_read_response *resp;
	struct ipc_header ipch;
	off_t offset;
	size_t cnt;
	int error;

	if (p == NULL)
		return (ERROR_INVALID);
	req = p;

	/*
	 * Note that we reuse p as a buffer.
	 */
	offset = req->offset;
	resp = p;
	error = fs->fs_ops->fs_directory_read(fs->fs_context, fsdc, &resp->entry, &offset, &cnt);
	if (error != 0) {
		ipch = IPC_HEADER_ERROR(reqh, error);

		error = ipc_port_send_data(&ipch, NULL, 0);
	} else {
		ipch = IPC_HEADER_REPLY(reqh);
		ipch.ipchdr_param = cnt;

		resp->next = offset;

		/*
		 * XXX
		 * We don't just send p directly because ipc_service is still
		 * messy about whether pages are freed or not, to say nothing
		 * of error handling.  Better to copy for now.
		 */
		error = ipc_port_send_data(&ipch, resp, sizeof *resp);
	}

	if (error != 0)
		return (error);
	return (0);
}

static int
fs_directory_ipc_close_handler(struct fs_directory *fsd, const struct ipc_header *reqh, void *p)
{
	fs_directory_context_t fsdc = fsd->fsd_context;
	struct fs *fs = fsd->fsd_fs;
	struct ipc_header ipch;
	int error;

	if (p != NULL)
		return (ERROR_INVALID);

	/*
	 * XXX
	 * Need to reference-count the service and only do this close once all
	 * users are done, and drop the receive right, and have dropping the
	 * receive right lead to garbage-collecting the port.
	 *
	 * At that point, do we even need a close(), or can the caller just drop
	 * the right to this port?  But then they need to refcount rights and be
	 * sure they only have this directory open once.
	 */

	error = fs->fs_ops->fs_directory_close(fs->fs_context, fsdc);
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

static int
fs_directory_ipc_service_start(struct fs_directory *fsd, ipc_port_t *portp)
{
	ipc_port_t port;
	int error;

	/*
	 * XXX
	 * Push all of this into one routine so then we can make the error reply
	 * easier?  fs_directory_service_start(&port, fsdc...);
	 */
	error = ipc_port_allocate(&port, IPC_PORT_FLAG_DEFAULT);
	if (error != 0)
		return (error);

	error = ipc_service(fsd->fsd_path, port, IPC_PORT_FLAG_DEFAULT,
			    fs_directory_ipc_handler, fsd);
	if (error != 0) {
		if (ipc_port_right_drop(port, IPC_PORT_RIGHT_RECEIVE) != 0)
			panic("%s: ipc_port_right_drop failed.", __func__);
		return (error);
	}

	*portp = port;

	return (0);
}
