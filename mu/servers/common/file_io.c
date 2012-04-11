#include <core/types.h>
#include <core/error.h>
#include <core/printf.h>
#include <core/string.h>
#include <fs/fs.h>
#include <ipc/ipc.h>
#include <ns/ns.h>
#include <vm/vm_page.h>

#include <common/common.h>

struct fs_wait {
	bool fw_done;
	int fw_error;
	ipc_port_t fw_port;
};

static void fs_open_file_response_handler(const struct ipc_dispatch *, const struct ipc_dispatch_handler *, const struct ipc_header *, void *);
static void fs_file_exec_response_handler(const struct ipc_dispatch *, const struct ipc_dispatch_handler *, const struct ipc_header *, void *);

int
open(ipc_port_t fs, const char *path, ipc_port_t *filep)
{
	const struct ipc_dispatch_handler *idh;
	struct fs_open_file_request fsreq;
	struct ipc_dispatch *id;
	struct fs_wait fw;
	int error;

	id = ipc_dispatch_allocate(IPC_PORT_UNKNOWN, IPC_PORT_FLAG_DEFAULT);
	idh = ipc_dispatch_register(id, fs_open_file_response_handler, &fw);

	memset(&fsreq, 0, sizeof fsreq);
	strlcpy(fsreq.path, path, sizeof fsreq.path);

	error = ipc_dispatch_send(id, idh, fs, FS_MSG_OPEN_FILE, IPC_PORT_RIGHT_SEND_ONCE, &fsreq, sizeof fsreq);
	if (error != 0)
		fatal("ipc_dispatch_send failed", error);

	fw.fw_done = false;
	fw.fw_error = 0;
	fw.fw_port = IPC_PORT_UNKNOWN;
	for (;;) {
		ipc_dispatch_poll(id);
		if (fw.fw_done)
			break;
		ipc_dispatch_wait(id);
	}

	*filep = fw.fw_port;
	return (fw.fw_error);
}

static void
fs_open_file_response_handler(const struct ipc_dispatch *id, const struct ipc_dispatch_handler *idh, const struct ipc_header *ipch, void *page)
{
	struct fs_wait *fw = idh->idh_softc;
	struct fs_open_file_response *fsresp;
	struct ipc_error_record *err;

	(void)id;

	switch (ipch->ipchdr_msg) {
	case IPC_MSG_REPLY(FS_MSG_OPEN_FILE):
		if (ipch->ipchdr_recsize != sizeof *fsresp || ipch->ipchdr_reccnt != 1 || page == NULL) {
			printf("Received message with unexpected data:\n");
			ipc_message_print(ipch, page);
			return;
		}
		fsresp = page;

		fw->fw_done = true;
		fw->fw_port = fsresp->file_port;
		return;
	case IPC_MSG_ERROR(FS_MSG_OPEN_FILE):
		if (ipch->ipchdr_recsize != sizeof *err || ipch->ipchdr_reccnt != 1 || page == NULL) {
			printf("Received message with unexpected data:\n");
			ipc_message_print(ipch, page);
			return;
		}
		err = page;

		fw->fw_done = true;
		fw->fw_error = err->error;
		return;
	default:
		printf("Received unexpected message:\n");
		ipc_message_print(ipch, page);
		return;
	}
}

int
exec(ipc_port_t file)
{
	const struct ipc_dispatch_handler *idh;
	struct ipc_dispatch *id;
	struct fs_wait fw;
	int error;

	id = ipc_dispatch_allocate(IPC_PORT_UNKNOWN, IPC_PORT_FLAG_DEFAULT);
	idh = ipc_dispatch_register(id, fs_file_exec_response_handler, &fw);

	error = ipc_dispatch_send(id, idh, file, FS_FILE_MSG_EXEC, IPC_PORT_RIGHT_SEND_ONCE, NULL, 0);
	if (error != 0)
		fatal("ipc_dispatch_send failed", error);

	fw.fw_done = false;
	fw.fw_error = 0;
	for (;;) {
		ipc_dispatch_poll(id);
		if (fw.fw_done)
			break;
		ipc_dispatch_wait(id);
	}

	return (fw.fw_error);
}

static void
fs_file_exec_response_handler(const struct ipc_dispatch *id, const struct ipc_dispatch_handler *idh, const struct ipc_header *ipch, void *page)
{
	struct fs_wait *fw = idh->idh_softc;
	struct ipc_error_record *err;

	(void)id;

	switch (ipch->ipchdr_msg) {
	case IPC_MSG_REPLY(FS_FILE_MSG_EXEC):
		if (ipch->ipchdr_recsize != 0 || ipch->ipchdr_reccnt != 0 || page != NULL) {
			printf("Received message with unexpected data:\n");
			ipc_message_print(ipch, page);
			return;
		}

		fw->fw_done = true;
		fw->fw_error = 0;
		return;
	case IPC_MSG_ERROR(FS_FILE_MSG_EXEC):
		if (ipch->ipchdr_recsize != sizeof *err || ipch->ipchdr_reccnt != 1 || page == NULL) {
			printf("Received message with unexpected data:\n");
			ipc_message_print(ipch, page);
			return;
		}
		err = page;

		fw->fw_done = true;
		fw->fw_error = err->error;
		return;
	default:
		printf("Received unexpected message:\n");
		ipc_message_print(ipch, page);
		return;
	}
}
