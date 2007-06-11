#include <core/types.h>
#include <core/error.h>
#include <core/ipc.h>
#include <core/startup.h>
#include <core/task.h>
#include <core/thread.h>

#include <io/device/console/console.h>

#define	NTHREADS	2

static struct test_private {
	struct thread *td;
	int i;
	ipc_port_t send;
	ipc_port_t receive;
} test_privates[NTHREADS];

static void
test_ipc_thread(void *arg)
{
	struct test_private *priv = arg;
	struct ipc_header hdr, rx;
	bool send;
	int error;

	ASSERT(priv != NULL, "Must have a private.");
	ASSERT(priv->td == current_thread(), "Must be on right thread.");

	hdr.ipchdr_src = priv->receive;
	hdr.ipchdr_dst = priv->send;
	hdr.ipchdr_type = NTHREADS;
	hdr.ipchdr_len = 0;

	send = true;

	for (;;) {
		if (send) {
			kcprintf("Worker %d is sending.\n", priv->i);
			error = ipc_port_send(&hdr, NULL);
			if (error != 0)
				panic("%s: ipc_port_send failed: %m", __func__,
				      error);
		}

		error = ipc_port_receive(priv->receive, &rx, NULL);
		if (error == ERROR_AGAIN) {
			send = false;
			ipc_port_wait(priv->receive);
			continue;
		}
		if (error != 0)
			panic("%s: ipc_port_receive failed: %m", __func__,
			      error);

		if (rx.ipchdr_type != NTHREADS)
			panic("%s: incorrect type.", __func__);
		
		kcprintf("Worker %d is receiving.\n", priv->i);

		hdr.ipchdr_dst = rx.ipchdr_src;

		send = true;
	}
}

static void
test_ipc_startup(void *arg)
{
	struct task *task;
	unsigned i;
	int error;

	for (i = 0; i < NTHREADS; i++) {
		test_privates[i].i = i;

		error = ipc_port_allocate(&test_privates[i].receive);
		if (error != 0)
			panic("%s: ipc_port_allocate failed: %m", __func__,
			      error);
		kcprintf("Worker %d rx port is %lu.\n", i, test_privates[i].receive);
		if (i != 0) {
			test_privates[i].send = test_privates[i - 1].receive;
			if (i == NTHREADS - 1)
				test_privates[0].send = test_privates[i].receive;
		}
	}

	error = task_create(&task, NULL, "test", TASK_DEFAULT | TASK_KERNEL);
	if (error != 0)
		panic("%s: task_create failed: %m", __func__, error);

	for (i = 0; i < NTHREADS; i++) {
		struct thread **tdp = &test_privates[i].td;
		error = thread_create(tdp, task, "thread i", THREAD_DEFAULT);
		if (error != 0)
			panic("%s: thread_create failed: %m", __func__, error);
		thread_set_upcall(*tdp, test_ipc_thread, &test_privates[i]);
		scheduler_thread_runnable(*tdp);
	}
}
STARTUP_ITEM(test_ipc, STARTUP_MAIN, STARTUP_BEFORE, test_ipc_startup, NULL);
