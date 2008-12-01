#include <core/types.h>
#include <core/cv.h>
#include <core/error.h>
#include <core/ipc.h>
#include <core/mutex.h>
#include <core/pool.h>
#include <core/queue.h>
#include <core/scheduler.h>
#include <core/startup.h>
#include <core/task.h>
#include <core/thread.h>
#include <io/console/console.h>
#include <ns/ns.h>

/*
 * This file is in two parts.  First is the Service Directory, which handles
 * the actual services information.  Second is the NS (name server) which
 * handles service registration and enumeration.
 */

struct service_directory_entry {
	char sde_name[NS_SERVICE_NAME_LENGTH];
	ipc_port_t sde_port;
	STAILQ_ENTRY(struct service_directory_entry) sde_link;
};

static STAILQ_HEAD(, struct service_directory_entry) service_directory;
static struct mutex service_directory_lock;
static struct pool service_directory_entry_pool;

#define	SERVICE_DIRECTORY_LOCK()	mutex_lock(&service_directory_lock)
#define	SERVICE_DIRECTORY_UNLOCK()	mutex_unlock(&service_directory_lock)

#if 0
static int service_directory_enter(const char *, ipc_port_t);
static int service_directory_lookup(const char *, ipc_port_t *);
#endif
static void service_directory_init(void);

static void
service_directory_init(void)
{
	int error;

	error = pool_create(&service_directory_entry_pool, "SERVICE DIRECTORY",
			    sizeof (struct service_directory_entry),
			    POOL_DEFAULT | POOL_VIRTUAL);
	if (error != 0)
		panic("%s: pool_create failed: %m", __func__, error);

	mutex_init(&service_directory_lock, "Service Directory", MUTEX_FLAG_DEFAULT);
	STAILQ_INIT(&service_directory);
}

static ipc_port_t ns_port;
static struct task *ns_task;
static struct thread *ns_thread;

static void
ns_main(void *arg)
{
	struct ipc_header ipch;
	int error;

	for (;;) {
		kcprintf("ns: waiting...\n");

		ipc_port_wait(ns_port);

		error = ipc_port_receive(ns_port, &ipch, NULL);
		if (error != 0) {
			if (error == ERROR_AGAIN)
				continue;
			panic("%s: ipc_port_receive failed: %m", __func__,
			      error);
		}

		kcprintf("ns: %lx -> %lx : %lx\n", ipch.ipchdr_src,
			 ipch.ipchdr_dst, ipch.ipchdr_type);
	}
}

static void
ns_startup(void *arg)
{
	int error;
	
	service_directory_init();

	error = task_create(&ns_task, NULL, "name server", TASK_KERNEL);
	if (error != 0)
		panic("%s: task_create failed: %m", __func__, error);

	error = thread_create(&ns_thread, ns_task, "ns main", THREAD_DEFAULT);
	if (error != 0)
		panic("%s: thread_create failed: %m", __func__, error);

	error = ipc_port_allocate_reserved(&ns_port, IPC_PORT_NS);
	if (error != 0)
		panic("%s: ipc_allocate_reserved failed: %m", __func__, error);

	thread_set_upcall(ns_thread, ns_main, NULL);
	scheduler_thread_runnable(ns_thread);
}
STARTUP_ITEM(ns, STARTUP_SERVERS, STARTUP_BEFORE, ns_startup, NULL);
