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
#include <fs/fs.h>
#include <ns/ns.h>

static ipc_port_t fs_port;
static struct task *fs_task;
static struct thread *fs_thread;

static void
fs_main(void *arg)
{
	struct ipc_header ipch;
	int error;

	/* Register with the NS.  */
	ipch.ipchdr_src = fs_port;
	ipch.ipchdr_dst = IPC_PORT_NS;
	ipch.ipchdr_msg = NS_MESSAGE_REGISTER;

	error = ipc_send(&ipch, NULL);
	if (error != 0)
		panic("%s: ipc_send failed: %m", __func__, error);

	for (;;) {
		kcprintf("fs: waiting for registration with ns...\n");

		ipc_port_wait(fs_port);

		error = ipc_port_receive(fs_port, &ipch, NULL);
		if (error != 0) {
			if (error == ERROR_AGAIN)
				continue;
		}

		kcprintf("fs: %lx -> %lx : %lx\n", ipch.ipchdr_src,
			 ipch.ipchdr_dst, ipch.ipchdr_msg);
	}

	/* Receive real requests and responses.  */

	for (;;) {
		kcprintf("fs: waiting...\n");

		ipc_port_wait(fs_port);

		error = ipc_port_receive(fs_port, &ipch, NULL);
		if (error != 0) {
			if (error == ERROR_AGAIN)
				continue;
			panic("%s: ipc_port_receive failed: %m", __func__,
			      error);
		}

		kcprintf("fs: %lx -> %lx : %lx\n", ipch.ipchdr_src,
			 ipch.ipchdr_dst, ipch.ipchdr_msg);
	}
}

static void
fs_startup(void *arg)
{
	int error;
	
	error = task_create(&fs_task, NULL, "file server", TASK_KERNEL);
	if (error != 0)
		panic("%s: task_create failed: %m", __func__, error);

	error = thread_create(&fs_thread, fs_task, "fs main", THREAD_DEFAULT);
	if (error != 0)
		panic("%s: thread_create failed: %m", __func__, error);

	error = ipc_port_allocate(&fs_port);
	if (error != 0)
		panic("%s: ipc_allocate failed: %m", __func__, error);

	thread_set_upcall(fs_thread, fs_main, NULL);
	scheduler_thread_runnable(fs_thread);
}
STARTUP_ITEM(fs, STARTUP_SERVERS, STARTUP_FIRST, fs_startup, NULL);
