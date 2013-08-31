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

int
fs_ipc_handler(void *softc, struct ipc_header *ipch, void *p)
{
	struct fs *fs = softc;

	switch (ipch->ipchdr_msg) {
	case FS_MSG_OPEN_FILE:
		return (fs_ipc_open_file_handler(fs, ipch, p));
	case FS_MSG_OPEN_DIRECTORY:
		return (fs_ipc_open_directory_handler(fs, ipch, p));
	default:
		/* Don't respond to nonsense.  */
		return (ERROR_INVALID);
	}
}
