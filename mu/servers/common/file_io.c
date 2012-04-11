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

struct fs_read_wait {
	bool frw_done;
	int frw_error;
	void *frw_buf;
	size_t frw_length;
};

static void fs_open_file_response_handler(const struct ipc_dispatch *, const struct ipc_dispatch_handler *, const struct ipc_header *, void *);
static void fs_file_read_response_handler(const struct ipc_dispatch *, const struct ipc_dispatch_handler *, const struct ipc_header *, void *);
static void fs_file_close_response_handler(const struct ipc_dispatch *, const struct ipc_dispatch_handler *, const struct ipc_header *, void *);
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

	ipc_dispatch_free(id);

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
			ipc_message_drop(ipch, page);
			return;
		}
		fsresp = page;

		fw->fw_done = true;
		fw->fw_port = fsresp->file_port;
		return;
	case IPC_MSG_ERROR(FS_MSG_OPEN_FILE):
		if (ipch->ipchdr_recsize != sizeof *err || ipch->ipchdr_reccnt != 1 || page == NULL) {
			ipc_message_drop(ipch, page);
			return;
		}
		err = page;

		fw->fw_done = true;
		fw->fw_error = err->error;
		return;
	default:
		ipc_message_drop(ipch, page);
		return;
	}
}

int
read(ipc_port_t file, void **bufp, off_t off, size_t *lenp)
{
	const struct ipc_dispatch_handler *idh;
	struct fs_file_read_request fsreq;
	struct ipc_dispatch *id;
	struct fs_read_wait frw;
	int error;

	id = ipc_dispatch_allocate(IPC_PORT_UNKNOWN, IPC_PORT_FLAG_DEFAULT);
	idh = ipc_dispatch_register(id, fs_file_read_response_handler, &frw);

	memset(&fsreq, 0, sizeof fsreq);
	fsreq.offset = off;
	fsreq.length = *lenp;

	error = ipc_dispatch_send(id, idh, file, FS_FILE_MSG_READ, IPC_PORT_RIGHT_SEND_ONCE, &fsreq, sizeof fsreq);
	if (error != 0)
		fatal("ipc_dispatch_send failed", error);

	frw.frw_done = false;
	frw.frw_error = 0;
	frw.frw_buf = NULL;
	frw.frw_length = 0;
	for (;;) {
		ipc_dispatch_poll(id);
		if (frw.frw_done)
			break;
		ipc_dispatch_wait(id);
	}
	
	if (frw.frw_error == 0) {
		*bufp = frw.frw_buf;
		*lenp = frw.frw_length;
	}

	ipc_dispatch_free(id);

	return (frw.frw_error);
}

static void
fs_file_read_response_handler(const struct ipc_dispatch *id, const struct ipc_dispatch_handler *idh, const struct ipc_header *ipch, void *page)
{
	struct fs_read_wait *frw = idh->idh_softc;
	struct ipc_error_record *err;

	(void)id;

	switch (ipch->ipchdr_msg) {
	case IPC_MSG_REPLY(FS_FILE_MSG_READ):
		frw->frw_done = true;
		frw->frw_error = 0;
		frw->frw_buf = page;
		frw->frw_length = ipch->ipchdr_recsize;
		return;
	case IPC_MSG_ERROR(FS_FILE_MSG_READ):
		if (ipch->ipchdr_recsize != sizeof *err || ipch->ipchdr_reccnt != 1 || page == NULL) {
			ipc_message_drop(ipch, page);
			return;
		}
		err = page;

		frw->frw_done = true;
		frw->frw_error = err->error;
		return;
	default:
		ipc_message_drop(ipch, page);
		return;
	}
}

int
close(ipc_port_t file)
{
	const struct ipc_dispatch_handler *idh;
	struct ipc_dispatch *id;
	struct fs_wait fw;
	int error;

	id = ipc_dispatch_allocate(IPC_PORT_UNKNOWN, IPC_PORT_FLAG_DEFAULT);
	idh = ipc_dispatch_register(id, fs_file_close_response_handler, &fw);

	error = ipc_dispatch_send(id, idh, file, FS_FILE_MSG_CLOSE, IPC_PORT_RIGHT_SEND_ONCE, NULL, 0);
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

	ipc_dispatch_free(id);

	return (fw.fw_error);
}

static void
fs_file_close_response_handler(const struct ipc_dispatch *id, const struct ipc_dispatch_handler *idh, const struct ipc_header *ipch, void *page)
{
	struct fs_wait *fw = idh->idh_softc;
	struct ipc_error_record *err;

	(void)id;

	switch (ipch->ipchdr_msg) {
	case IPC_MSG_REPLY(FS_FILE_MSG_CLOSE):
		if (ipch->ipchdr_recsize != 0 || ipch->ipchdr_reccnt != 0 || page != NULL) {
			ipc_message_drop(ipch, page);
			return;
		}

		fw->fw_done = true;
		fw->fw_error = 0;
		return;
	case IPC_MSG_ERROR(FS_FILE_MSG_CLOSE):
		if (ipch->ipchdr_recsize != sizeof *err || ipch->ipchdr_reccnt != 1 || page == NULL) {
			ipc_message_drop(ipch, page);
			return;
		}
		err = page;

		fw->fw_done = true;
		fw->fw_error = err->error;
		return;
	default:
		ipc_message_drop(ipch, page);
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

	ipc_dispatch_free(id);

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
			ipc_message_drop(ipch, page);
			return;
		}

		fw->fw_done = true;
		fw->fw_error = 0;
		return;
	case IPC_MSG_ERROR(FS_FILE_MSG_EXEC):
		if (ipch->ipchdr_recsize != sizeof *err || ipch->ipchdr_reccnt != 1 || page == NULL) {
			ipc_message_drop(ipch, page);
			return;
		}
		err = page;

		fw->fw_done = true;
		fw->fw_error = err->error;
		return;
	default:
		ipc_message_drop(ipch, page);
		return;
	}
}
