#include <core/types.h>
#include <core/cv.h>
#include <core/error.h>
#include <core/mutex.h>
#include <core/pool.h>
#include <core/queue.h>
#include <core/scheduler.h>
#include <core/startup.h>
#include <core/task.h>
#include <core/thread.h>
#include <io/console/console.h>
#include <ipc/ipc.h>
#include <ipc/port.h>
#include <ipc/service.h>
#include <ns/ns.h>
#include <ns/service_directory.h>

static int ns_handler(void *, struct ipc_header *);

static int
ns_handler(void *arg, struct ipc_header *ipch)
{
	int error;

	IPC_HEADER_REPLY(ipch);

	error = ipc_port_send(ipch);
	if (error != 0)
		panic("%s: ipc_port_send failed: %m", __func__, error);

	return (0);
}

static void
ns_startup(void *arg)
{
	int error;
	
	service_directory_init();

	error = ipc_service("ns", IPC_PORT_NS, ns_handler, NULL);
	if (error != 0)
		panic("%s: ipc_service failed: %m", __func__, error);
}
STARTUP_ITEM(ns, STARTUP_SERVERS, STARTUP_BEFORE, ns_startup, NULL);
