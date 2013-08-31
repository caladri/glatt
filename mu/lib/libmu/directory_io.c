#include <core/types.h>
#include <core/error.h>
#include <core/string.h>
#include <fs/fs.h>
#include <fs/fs_ipc.h>
#include <ipc/ipc.h>

#include <libmu/common.h>
#include <libmu/ipc_request.h>
#include <libmu/process.h>

int
opendir(ipc_port_t fs, const char *path, ipc_port_t *filep)
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
	req.msg = FS_MSG_OPEN_DIRECTORY;
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
readdir(ipc_port_t file, struct fs_directory_entry *de, off_t *offp)
{
	struct ipc_request_message req;
	struct ipc_response_message resp;
	struct fs_directory_read_request fsreq;
	struct fs_directory_read_response *fsresp;
	int error;

	memset(&req, 0, sizeof req);
	memset(&resp, 0, sizeof resp);
	memset(&fsreq, 0, sizeof fsreq);

	fsreq.offset = *offp;

	req.src = IPC_PORT_UNKNOWN;
	req.dst = file;
	req.msg = FS_DIRECTORY_MSG_READ;
	req.param = 0;
	req.data = &fsreq;
	req.datalen = sizeof fsreq;

	resp.data = true;

	error = ipc_request(&req, &resp);
	if (error != 0)
		return (error);

	if (resp.error != 0)
		return (resp.error);

	/* End of directory.  */
	if (resp.param == 0) {
		de->name[0] = '\0';
		*offp = 0;
		return (0);
	}

	/* XXX assert cnt == 1?  */

	fsresp = resp.page;
	strlcpy(de->name, fsresp->entry.name, sizeof de->name);
	*offp = fsresp->next;

	vm_page_free(resp.page);

	return (0);
}

int
closedir(ipc_port_t file)
{
	struct ipc_request_message req;
	struct ipc_response_message resp;
	int error;

	memset(&req, 0, sizeof req);
	memset(&resp, 0, sizeof resp);

	req.src = IPC_PORT_UNKNOWN;
	req.dst = file;
	req.msg = FS_DIRECTORY_MSG_CLOSE;
	req.param = 0;

	resp.data = false;

	error = ipc_request(&req, &resp);
	if (error != 0)
		return (error);

	if (resp.error != 0)
		return (resp.error);

	return (0);
}
