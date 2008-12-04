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
#include <ns/service_directory.h>

static struct task *ns_task;
static struct thread *ns_thread;

static void
ns_main(void *arg)
{
	struct ipc_header ipch;
	int error;

	for (;;) {
		kcprintf("ns: waiting...\n");

		ipc_port_wait(IPC_PORT_NS);

		error = ipc_port_receive(IPC_PORT_NS, &ipch, NULL);
		if (error != 0) {
			if (error == ERROR_AGAIN)
				continue;
			panic("%s: ipc_port_receive failed: %m", __func__,
			      error);
		}

		kcprintf("ns: %lx -> %lx : %lx\n", ipch.ipchdr_src,
			 ipch.ipchdr_dst, ipch.ipchdr_msg);

		IPC_HEADER_REPLY(&ipch);

		error = ipc_send(&ipch, NULL);
		if (error != 0)
			panic("%s: ipc_port_send failed: %m", __func__, error);
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

	error = ipc_port_allocate_reserved(ns_task, IPC_PORT_NS);
	if (error != 0)
		panic("%s: ipc_allocate_reserved failed: %m", __func__, error);

	thread_set_upcall(ns_thread, ns_main, NULL);
	scheduler_thread_runnable(ns_thread);
}
STARTUP_ITEM(ns, STARTUP_SERVERS, STARTUP_BEFORE, ns_startup, NULL);
