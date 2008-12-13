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
#include <ipc/data.h>
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

static void ipc_service_dump(struct ipc_service_context *, struct ipc_header *,
			     struct ipc_data *);
static void ipc_service_main(void *);

int
ipc_service(const char *name, ipc_port_t port, ipc_service_t *handler,
	    void *arg)
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
	
	error = task_create(&ipcsc->ipcsc_task, NULL, ipcsc->ipcsc_name,
			    TASK_KERNEL);
	if (error != 0)
		panic("%s: task_create failed: %m", __func__, error);

	error = thread_create(&ipcsc->ipcsc_thread, ipcsc->ipcsc_task,
			      "ipc service", THREAD_DEFAULT);
	if (error != 0)
		panic("%s: thread_create failed: %m", __func__, error);

	switch (ipcsc->ipcsc_port) {
	case IPC_PORT_UNKNOWN:
		error = ipc_port_allocate(ipcsc->ipcsc_task,
					  &ipcsc->ipcsc_port);
		if (error != 0)
			panic("%s: ipc_allocate failed: %m", __func__, error);
		break;
	default:
		error = ipc_port_allocate_reserved(ipcsc->ipcsc_task,
						   ipcsc->ipcsc_port);
		if (error != 0)
			panic("%s: ipc_allocate_reserved failed: %m", __func__,
			      error);
		break;
	}

	thread_set_upcall(ipcsc->ipcsc_thread, ipc_service_main, ipcsc);
	scheduler_thread_runnable(ipcsc->ipcsc_thread);

	return (0);
}

static void
ipc_service_dump(struct ipc_service_context *ipcsc, struct ipc_header *ipch,
		 struct ipc_data *ipcd)
{
	kcprintf("%s: %lx -> %lx : %lx", ipcsc->ipcsc_name,
		 ipch->ipchdr_src, ipch->ipchdr_dst, ipch->ipchdr_msg);
	if (ipcd != NULL) {
		kcprintf(" (data");
		while (ipcd != NULL) {
			kcprintf(" <%x, %p, %zu>", ipcd->ipcd_type,
				 ipcd->ipcd_addr, ipcd->ipcd_len);
			ipcd = ipcd->ipcd_next;
		}
		kcprintf(")");
	}
	kcprintf("\n");
}

static void
ipc_service_main(void *arg)
{
	struct ipc_service_context *ipcsc = arg;
	struct ipc_header ipch;
	struct ipc_data ipcd, *ipcdp;
	int error;

	/* Register with the NS.  */
	if (ipcsc->ipcsc_port >= IPC_PORT_UNRESERVED_START) {
		struct ns_register_request nsreq;
		struct ns_register_response *nsresp;

		ipch.ipchdr_src = ipcsc->ipcsc_port;
		ipch.ipchdr_dst = IPC_PORT_NS;
		ipch.ipchdr_right = IPC_PORT_RIGHT_SEND_ONCE;
		ipch.ipchdr_msg = NS_MESSAGE_REGISTER;

		nsreq.error = 0;
		nsreq.cookie = 0;
		strlcpy(nsreq.service_name, ipcsc->ipcsc_name,
			NS_SERVICE_NAME_LENGTH);
		nsreq.port = ipcsc->ipcsc_port;

		ipcd.ipcd_type = IPC_DATA_TYPE_SMALL;
		ipcd.ipcd_addr = &nsreq;
		ipcd.ipcd_len = sizeof nsreq;
		ipcd.ipcd_next = NULL;

		error = ipc_port_send(&ipch, &ipcd);
		if (error != 0) {
			ipc_service_dump(ipcsc, &ipch, &ipcd);
			panic("%s: ipc_send failed: %m", __func__, error);
		}

		for (;;) {
			kcprintf("%s: waiting for registration with ns...\n",
				 ipcsc->ipcsc_name);

			error = ipc_port_wait(ipcsc->ipcsc_port);
			if (error != 0)
				panic("%s: ipc_port_wait failed: %m", __func__,
				      error);

			error = ipc_port_receive(ipcsc->ipcsc_port, &ipch,
						 &ipcdp);
			if (error != 0) {
				if (error == ERROR_AGAIN)
					continue;
			}

			ipc_service_dump(ipcsc, &ipch, ipcdp);

			if (ipch.ipchdr_src != IPC_PORT_NS) {
				kcprintf("%s: message from unexpected source.\n",
					 ipcsc->ipcsc_name);
				continue;
			}

			if (ipch.ipchdr_msg != IPC_MSG_REPLY(NS_MESSAGE_REGISTER)) {
				kcprintf("%s: unexpected message type from ns.\n",
					 ipcsc->ipcsc_name);
				continue;
			}

			if (ipcdp == NULL ||
			    ipcdp->ipcd_type != IPC_DATA_TYPE_SMALL ||
			    ipcdp->ipcd_addr == NULL ||
			    ipcdp->ipcd_len != sizeof *nsresp ||
			    ipcdp->ipcd_next != NULL) {
				kcprintf("%s: gibberish data from ns.\n",
					 ipcsc->ipcsc_name);
				continue;
			}

			nsresp = ipcdp->ipcd_addr;

			if (nsresp->error != 0 || nsresp->cookie != 0 ||
			    strcmp(ipcsc->ipcsc_name, nsresp->service_name) != 0 ||
			    nsresp->port != ipcsc->ipcsc_port) {
				panic("%s: unexpected response from ns.",
				      ipcsc->ipcsc_name);
			}

			break;
		}

		kcprintf("%s: registered with ns.\n", ipcsc->ipcsc_name);
	}

	/* Receive real requests and responses.  */

	for (;;) {
		kcprintf("%s: waiting...\n", ipcsc->ipcsc_name);

		error = ipc_port_wait(ipcsc->ipcsc_port);
		if (error != 0)
			panic ("%s: ipc_port_wait failed: %m", __func__, error);

		error = ipc_port_receive(ipcsc->ipcsc_port, &ipch, &ipcdp);
		if (error != 0) {
			if (error == ERROR_AGAIN)
				continue;
			panic("%s: ipc_port_receive failed: %m", __func__,
			      error);
		}

		ipc_service_dump(ipcsc, &ipch, ipcdp);

		ipcsc->ipcsc_handler(ipcsc->ipcsc_arg, &ipch, ipcdp);
		
		if (ipcdp != NULL)
			ipc_data_free(ipcdp);
	}
}
