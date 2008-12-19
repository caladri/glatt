#include <core/types.h>
#include <core/error.h>
#include <core/startup.h>
#include <ipc/ipc.h>
#include <ipc/service.h>
#include <fs/fs.h>

static int fs_handler(void *, struct ipc_header *, struct ipc_data *);

static int
fs_handler(void *arg, struct ipc_header *ipch, struct ipc_data *ipcd)
{
	return (0);
}

static void
fs_startup(void *arg)
{
	int error;

	error = ipc_service("fs", IPC_PORT_UNKNOWN, fs_handler, NULL);
	if (error != 0)
		panic("%s: ipc_service failed: %m", __func__, error);
}
STARTUP_ITEM(fs, STARTUP_SERVERS, STARTUP_FIRST, fs_startup, NULL);
