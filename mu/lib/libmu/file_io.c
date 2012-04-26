#include <core/types.h>
#include <core/error.h>
#include <core/printf.h>
#include <core/string.h>
#include <fs/fs.h>
#include <ipc/ipc.h>
#include <ns/ns.h>
#include <vm/vm_page.h>

#include <libmu/common.h>
#include <libmu/ipc_request.h>
#include <libmu/process.h>

int
open(ipc_port_t fs, const char *path, ipc_port_t *filep)
{
	struct ipc_request_message req;
	struct ipc_response_message resp;
	struct fs_open_file_request fsreq;
	int error;

	memset(&req, 0, sizeof req);
	memset(&resp, 0, sizeof resp);
	memset(&fsreq, 0, sizeof fsreq);

	strlcpy(fsreq.path, path, sizeof fsreq.path);

	req.src = IPC_PORT_UNKNOWN;
	req.dst = fs;
	req.msg = FS_MSG_OPEN_FILE;
	req.param = 0;
	req.data = &fsreq;
	req.datalen = sizeof fsreq;

	resp.data = false;

	error = ipc_request(&req, &resp);
	if (error != 0)
		return (error);

	if (resp.error != 0)
		return (resp.error);

	*filep = resp.param;

	return (0);
}

int
read(ipc_port_t file, void **bufp, off_t off, size_t *lenp)
{
	struct ipc_request_message req;
	struct ipc_response_message resp;
	struct fs_file_read_request fsreq;
	int error;

	memset(&req, 0, sizeof req);
	memset(&resp, 0, sizeof resp);
	memset(&fsreq, 0, sizeof fsreq);

	fsreq.offset = off;
	fsreq.length = *lenp;

	req.src = IPC_PORT_UNKNOWN;
	req.dst = file;
	req.msg = FS_FILE_MSG_READ;
	req.param = 0;
	req.data = &fsreq;
	req.datalen = sizeof fsreq;

	resp.data = true;

	error = ipc_request(&req, &resp);
	if (error != 0)
		return (error);

	if (resp.error != 0)
		return (resp.error);

	*bufp = resp.page;
	*lenp = resp.param;

	return (0);
}

int
close(ipc_port_t file)
{
	struct ipc_request_message req;
	struct ipc_response_message resp;
	int error;

	memset(&req, 0, sizeof req);
	memset(&resp, 0, sizeof resp);

	req.src = IPC_PORT_UNKNOWN;
	req.dst = file;
	req.msg = FS_FILE_MSG_CLOSE;
	req.param = 0;

	resp.data = false;

	error = ipc_request(&req, &resp);
	if (error != 0)
		return (error);

	if (resp.error != 0)
		return (resp.error);

	return (0);
}

int
exec(ipc_port_t file, ipc_port_t *taskp, unsigned argc, const char **argv)
{
	struct ipc_request_message req;
	struct ipc_response_message resp;
	uintptr_t *reloc;
	const char *arg;
	ipc_port_t task;
	char *argp;
	unsigned i;
	void *page;
	int error;

	memset(&req, 0, sizeof req);
	memset(&resp, 0, sizeof resp);

	req.src = IPC_PORT_UNKNOWN;
	req.dst = file;
	req.msg = FS_FILE_MSG_EXEC;
	req.param = 0;

	resp.data = false;

	error = ipc_request(&req, &resp);
	if (error != 0)
		return (error);

	if (resp.error != 0)
		return (resp.error);

	/*
	 * Now send process information to the task port.
	 */
	task = resp.param;

	memset(&req, 0, sizeof req);
	memset(&resp, 0, sizeof resp);

	req.src = IPC_PORT_UNKNOWN;
	req.dst = task;
	req.msg = PROCESS_MSG_START;
	req.param = argc;

	resp.data = false;

	if (argc != 0) {
		error = vm_page_get(&page);
		if (error != 0) {
			/*
			 * XXX
			 * Teardown task?
			 */
			return (error);
		}
		reloc = page;
		argp = (char *)page + (argc * sizeof argp);
		/* XXX Bounds check.  */
		for (i = 0; i < argc; i++) {
			reloc[i] = argp - (char *)page;
			arg = argv[i];
			while ((*argp++ = *arg++) != '\0')
				continue;
		}
		req.page = page;
	}

	error = ipc_request(&req, &resp);
	if (error != 0) {
		/*
		 * XXX
		 * Teardown task?
		 */
		return (error);
	}

	if (resp.error != 0) {
		/*
		 * XXX
		 * Teardown task?
		 */
		return (resp.error);
	}

	if (taskp != NULL)
		*taskp = task;

	return (0);
}
