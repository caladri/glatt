#include <core/types.h>
#include <core/error.h>
#include <core/ipc.h>
#include <core/startup.h>
#include <core/task.h>
#include <core/thread.h>

#include <io/device/console/console.h>

#define	NTHREADS	(13)

static struct test_private {
	struct task *task;
	struct thread *td;
	int i;
	ipc_msg_t last;
	ipc_port_t send;
	ipc_port_t receive;
} test_privates[NTHREADS];

static void
test_ipc_status(void)
{
	unsigned i;

	for (i = 0; i < NTHREADS; i++) {
		kcprintf("%s%d%s", i == 0 ? "[" : ",", test_privates[i].last,
			 i == NTHREADS - 1 ? "]\n" : "");
	}
}

static void
test_ipc_send(struct test_private *priv, int type)
{
	struct ipc_header hdr;
	int error;

	hdr.ipchdr_src = priv->receive;
	hdr.ipchdr_dst = priv->send;
	hdr.ipchdr_type = type;
	hdr.ipchdr_len = 0;

	error = ipc_send(&hdr, NULL);
	if (error != 0)
		panic("%s: ipc_send failed: %m", __func__, error);
}

static void
test_ipc_receive(struct test_private *priv, struct ipc_header *rx)
{
	int error;

	for (;;) {
		error = ipc_port_receive(priv->receive, rx, NULL);
		if (error == 0)
			break;
		if (error != ERROR_AGAIN)
			panic("%s: ipc_port_receive failed: %m",
			      __func__, error);
		ipc_port_wait(priv->receive);
	}
	if (rx->ipchdr_dst != priv->receive)
		panic("%s: incorrect destination.", __func__);
}

static void
test_ipc_thread(void *arg)
{
	struct test_private *priv = arg;
	struct ipc_header rx;
	bool send;

	ASSERT(priv->td == current_thread(), "Must be on right thread.");

	priv->last = priv->i;

	if (priv->i == 0) {
		test_ipc_send(priv, priv->i);
		send = false;
	}
	for (;;) {
		if (priv->i == 0)
			test_ipc_status();
		test_ipc_receive(priv, &rx);
		if (priv->i == 0) {
			priv->last = rx.ipchdr_type;
			test_ipc_send(priv, priv->last);
		} else {
			test_ipc_send(priv, priv->last);
			priv->last = rx.ipchdr_type;
		}
	}
}

static void
test_ipc_startup_thread(void *arg)
{
	struct task *parent = arg;
	unsigned i;
	int error;

	for (i = 0; i < NTHREADS; i++) {
		test_privates[i].i = i;

		error = ipc_port_allocate(&test_privates[i].receive);
		if (error != 0)
			panic("%s: ipc_port_allocate failed: %m", __func__,
			      error);
		if (i != 0) {
			test_privates[i].send = test_privates[i - 1].receive;
			if (i == NTHREADS - 1)
				test_privates[0].send = test_privates[i].receive;
		}
	}

	for (i = 0; i < NTHREADS; i++) {
		struct task **taskp = &test_privates[i].task;
		struct thread **tdp = &test_privates[i].td;

		error = task_create(taskp, parent, "test i",
				    TASK_DEFAULT | TASK_KERNEL);
		if (error != 0)
			panic("%s: task_create failed: %m", __func__, error);

		error = thread_create(tdp, *taskp, "test thread",
				      THREAD_DEFAULT);
		if (error != 0)
			panic("%s: thread_create failed: %m", __func__, error);
		thread_set_upcall(*tdp, test_ipc_thread, &test_privates[i]);
		scheduler_thread_runnable(*tdp);
	}
	scheduler_thread_sleeping(current_thread());
	scheduler_schedule(NULL);
	panic("%s: scheduler_schedule returned!", __func__);
}

static void
test_ipc_startup(void *arg)
{
	struct thread *td;
	struct task *task;
	int error;

	error = task_create(&task, NULL, "test startup",
			    TASK_DEFAULT | TASK_KERNEL);
	if (error != 0)
		panic("%s: task_create failed: %m", __func__, error);

	error = thread_create(&td, task, "test startup", THREAD_DEFAULT);
	if (error != 0)
		panic("%s: thread_create failed: %m", __func__, error);
	thread_set_upcall(td, test_ipc_startup_thread, task);
	scheduler_thread_runnable(td);
}
STARTUP_ITEM(test_ipc, STARTUP_MAIN, STARTUP_BEFORE, test_ipc_startup, NULL);
