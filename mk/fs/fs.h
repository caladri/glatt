#ifndef	_FS_FS_H_
#define	_FS_FS_H_

#include <ipc/ipc.h>
#include <vm/vm_page.h>

/*
 * Global filesystem parameters.
 */
#define	FS_PATH_MAX		(PAGE_SIZE)

/*
 * XXX Filesystem discovery IPC?
 */

/*
 * Filesystem IPC.
 */

#define	FS_MSG_OPEN_FILE	(0)

struct fs_open_file_request {
	char name[FS_PATH_MAX];
};

struct fs_open_file_response {
	ipc_port_t file_port;
};

struct fs_open_file_error {
	int error;
};

/*
 * File IPC.
 */

#define	FS_FILE_MSG_READ	(0)
#define	FS_FILE_MSG_CLOSE	(1)

struct fs_file_read_request {
	off_t offset;
	size_t len;
};

struct fs_file_read_error {
	int error;
};

struct fs_file_close_error {
	int error;
};

#endif /* !_FS_FS_H_ */
