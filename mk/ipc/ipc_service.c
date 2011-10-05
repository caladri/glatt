#include <core/types.h>
#include <core/cv.h>
#include <core/error.h>
#include <core/malloc.h>
#include <core/mutex.h>
#include <core/pool.h>
#include <core/queue.h>
#include <core/scheduler.h>
#include <core/string.h>
#include <core/task.h>
#include <core/thread.h>
#include <io/console/console.h>
#include <ipc/ipc.h>
#include <ipc/port.h>
#include <ipc/service.h>
#include <ns/ns.h>

/*
 * Notes:
 *
 * There will only be a few IPC Services in the microkernel, so we will not be
 * putting them in a pool.  If the balance shifts (e.g. to one service per
 * device), a pool may be appropriate.
 *
 * Support for multiple threads would be nice.
 */

struct ipc_service_context {
	const char *ipcsc_name;
	ipc_service_t *ipcsc_handler;
	void *ipcsc_arg;
	ipc_port_t ipcsc_port;
	struct task *ipcsc_task;
	struct thread *ipcsc_thread;
};

#ifdef VERBOSE
static void ipc_service_dump(const struct ipc_service_context *, const struct ipc_header *);
#endif
static void ipc_service_main(void *);

int
ipc_service(const char *name, ipc_port_t port, ipc_port_flags_t flags,
	    ipc_service_t *handler, void *arg)
{
	struct ipc_service_context *ipcsc;
	int error;

	ipcsc = malloc(sizeof *ipcsc);
	ipcsc->ipcsc_name = name;
	ipcsc->ipcsc_handler = handler;
	ipcsc->ipcsc_arg = arg;
	ipcsc->ipcsc_port = port;
	ipcsc->ipcsc_task = NULL;
	ipcsc->ipcsc_thread = NULL;

	error = task_create(&ipcsc->ipcsc_task, ipcsc->ipcsc_name, TASK_KERNEL);
	if (error != 0)
		panic("%s: task_create failed: %m", __func__, error);

	error = thread_create(&ipcsc->ipcsc_thread, ipcsc->ipcsc_task,
			      "ipc service", THREAD_DEFAULT);
	if (error != 0)
		panic("%s: thread_create failed: %m", __func__, error);

	switch (ipcsc->ipcsc_port) {
	case IPC_PORT_UNKNOWN:
		error = ipc_port_allocate(ipcsc->ipcsc_task,
					  &ipcsc->ipcsc_port, flags);
		if (error != 0)
			panic("%s: ipc_allocate failed: %m", __func__, error);
		break;
	default:
		error = ipc_port_allocate_reserved(ipcsc->ipcsc_task,
						   ipcsc->ipcsc_port, flags);
		if (error != 0)
			panic("%s: ipc_allocate_reserved failed: %m", __func__,
			      error);
		break;
	}

	thread_set_upcall(ipcsc->ipcsc_thread, ipc_service_main, ipcsc);
	scheduler_thread_runnable(ipcsc->ipcsc_thread);

	return (0);
}

#ifdef VERBOSE
static void
ipc_service_dump(const struct ipc_service_context *ipcsc, const struct ipc_header *ipch)
{
	/*
	 * XXX
	 * Dump new fields.
	 */
	kcprintf("%s: %lx -> %lx : %lx\n", ipcsc->ipcsc_name,
		 ipch->ipchdr_src, ipch->ipchdr_dst, ipch->ipchdr_msg);
}
#endif

static void
ipc_service_main(void *arg)
{
	struct ipc_service_context *ipcsc = arg;
	struct ipc_header ipch;
	int error;
	void *p;

	/* Register with the NS.  */
	if (ipcsc->ipcsc_port >= IPC_PORT_UNRESERVED_START) {
		struct ns_register_request nsreq;
		struct ns_register_response *nsresp;

		ipch.ipchdr_src = ipcsc->ipcsc_port;
		ipch.ipchdr_dst = IPC_PORT_NS;
		ipch.ipchdr_msg = NS_MESSAGE_REGISTER;
		ipch.ipchdr_right = IPC_PORT_RIGHT_SEND_ONCE;
		ipch.ipchdr_cookie = 0;
		ipch.ipchdr_recsize = sizeof nsreq;
		ipch.ipchdr_reccnt = 1;

		nsreq.error = 0;
		strlcpy(nsreq.service_name, ipcsc->ipcsc_name,
			NS_SERVICE_NAME_LENGTH);
		nsreq.port = ipcsc->ipcsc_port;

		error = ipc_port_send_data(&ipch, &nsreq, sizeof nsreq);
		if (error != 0) {
#ifdef VERBOSE
			ipc_service_dump(ipcsc, &ipch);
#endif
			panic("%s: ipc_send failed: %m", __func__, error);
		}

		for (;;) {
#ifdef VERBOSE
			kcprintf("%s: waiting for registration with ns...\n",
				 ipcsc->ipcsc_name);
#endif

			error = ipc_port_wait(ipcsc->ipcsc_port);
			if (error != 0)
				panic("%s: ipc_port_wait failed: %m", __func__,
				      error);

			error = ipc_port_receive(ipcsc->ipcsc_port, &ipch, &p);
			if (error != 0) {
				if (error == ERROR_AGAIN)
					continue;
			}

#ifdef VERBOSE
			ipc_service_dump(ipcsc, &ipch);
#endif

			if (ipch.ipchdr_src != IPC_PORT_NS) {
#ifdef VERBOSE
				kcprintf("%s: message from unexpected source.\n",
					 ipcsc->ipcsc_name);
#endif
				continue;
			}

			if (ipch.ipchdr_msg != IPC_MSG_REPLY(NS_MESSAGE_REGISTER)) {
#ifdef VERBOSE
				kcprintf("%s: unexpected message type from ns.\n",
					 ipcsc->ipcsc_name);
#endif
				continue;
			}

			if (ipch.ipchdr_cookie != 0) {
#ifdef VERBOSE
				kcprintf("%s: unexpected cookie from ns.\n",
					 ipcsc->ipcsc_name);
#endif
				continue;
			}

			if (ipch.ipchdr_recsize != sizeof *nsresp) {
#ifdef VERBOSE
				kcprintf("%s: response record from ns has wrong size.\n",
					 ipcsc->ipcsc_name);
#endif
				continue;
			}

			if (ipch.ipchdr_reccnt != 1) {
#ifdef VERBOSE
				kcprintf("%s: wrong number of response records from ns.\n",
					 ipcsc->ipcsc_name);
#endif
				continue;
			}

			if (p == NULL) {
#ifdef VERBOSE
				kcprintf("%s: no data from ns.\n",
					 ipcsc->ipcsc_name);
#endif
				continue;
			}

			nsresp = p;

			if (nsresp->error != 0 ||
			    strcmp(ipcsc->ipcsc_name, nsresp->service_name) != 0 ||
			    nsresp->port != ipcsc->ipcsc_port) {
				panic("%s: unexpected response from ns.",
				      ipcsc->ipcsc_name);
			}

			/* XXX Free page in p.  */

			break;
		}

#ifdef VERBOSE
		kcprintf("%s: registered with ns.\n", ipcsc->ipcsc_name);
#endif
	}

	/* Receive real requests and responses.  */

	for (;;) {
#ifdef VERBOSE
		kcprintf("%s: waiting...\n", ipcsc->ipcsc_name);
#endif

		error = ipc_port_wait(ipcsc->ipcsc_port);
		if (error != 0)
			panic("%s: ipc_port_wait failed: %m", __func__, error);

		error = ipc_port_receive(ipcsc->ipcsc_port, &ipch, &p);
		if (error != 0) {
			if (error == ERROR_AGAIN)
				continue;
			panic("%s: ipc_port_receive failed: %m", __func__,
			      error);
		}

#ifdef VERBOSE
		ipc_service_dump(ipcsc, &ipch);
#endif

		error = ipcsc->ipcsc_handler(ipcsc->ipcsc_arg, &ipch, p);
		if (error != 0)
			kcprintf("%s: service handler failed: %m\n", __func__, error);

		if (p != NULL) {
			/* XXX Free page in p?  */
		}
	}
}
