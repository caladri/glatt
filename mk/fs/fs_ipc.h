#ifndef	_FS_FS_IPC_H_
#define	_FS_FS_IPC_H_

#include <ipc/ipc.h>
#include <vm/vm_page.h>

#ifdef MK
struct fs;
#endif

/*
 * XXX Filesystem discovery IPC?
 */

/*
 * Filesystem IPC.
 */

#define	FS_MSG_OPEN_FILE	(1)
#define	FS_MSG_OPEN_DIRECTORY	(1)

struct fs_open_file_request {
	char path[FS_PATH_MAX];
};

#ifdef MK
ipc_service_t fs_ipc_handler;
int fs_ipc_open_file_handler(struct fs *, const struct ipc_header *, void *);
int fs_ipc_open_directory_handler(struct fs *, const struct ipc_header *, void *);
#endif

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

/*
 * Directory IPC.
 */

#define	FS_DIRECTORY_MSG_READ	(1)
#define	FS_DIRECTORY_MSG_CLOSE	(2)

struct fs_directory_read_request {
	off_t offset;
};

/* XXX Return multiple entries in each page.  */
struct fs_directory_read_response {
	off_t next;
	struct fs_directory_entry entry;
};

#endif /* !_FS_FS_IPC_H_ */
