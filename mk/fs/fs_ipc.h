#ifndef	_FS_FS_IPC_H_
#define	_FS_FS_IPC_H_

#include <ipc/ipc.h>
#include <vm/vm_page.h>

/*
 * XXX Filesystem discovery IPC?
 */

/*
 * Filesystem IPC.
 */

#define	FS_MSG_OPEN_FILE	(1)

struct fs_open_file_request {
	char path[FS_PATH_MAX];
};

/*
 * File IPC.
 */

#define	FS_FILE_MSG_READ	(1)
#define	FS_FILE_MSG_CLOSE	(2)
#define	FS_FILE_MSG_EXEC	(3)

struct fs_file_read_request {
	off_t offset;
	size_t length;
};

#ifdef MK
ipc_service_t fs_ipc_handler;
#endif

#endif /* !_FS_FS_IPC_H_ */
