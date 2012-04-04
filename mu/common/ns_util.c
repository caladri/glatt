#include <core/types.h>
#include <core/error.h>
#include <core/printf.h>
#include <core/string.h>
#include <ipc/ipc.h>
#include <ns/ns.h>
#include <vm/vm_page.h>

#include <common/common.h>

struct ns_response_wait {
	bool nrw_done;
	int nrw_error;
	ipc_port_t nrw_port;
};

static void ns_lookup_response_handler(const struct ipc_dispatch *, const struct ipc_dispatch_handler *, const struct ipc_header *, void *);
static void ns_register_response_handler(const struct ipc_dispatch *, const struct ipc_dispatch_handler *, const struct ipc_header *, void *);

ipc_port_t
ns_lookup(const char *service)
{
	const struct ipc_dispatch_handler *idh;
	struct ns_lookup_request nsreq;
	struct ns_response_wait nrw;
	struct ipc_dispatch *id;
	int error;

	id = ipc_dispatch_allocate(IPC_PORT_UNKNOWN, IPC_PORT_FLAG_DEFAULT);
	idh = ipc_dispatch_register(id, ns_lookup_response_handler, &nrw);

	nsreq.error = 0;
	memset(nsreq.service_name, 0, NS_SERVICE_NAME_LENGTH);
	strlcpy(nsreq.service_name, service, NS_SERVICE_NAME_LENGTH);

	error = ipc_dispatch_send(id, idh, IPC_PORT_NS, NS_MESSAGE_LOOKUP, IPC_PORT_RIGHT_SEND_ONCE, &nsreq, sizeof nsreq);
	if (error != 0)
		fatal("ipc_dispatch_send failed", error);

	nrw.nrw_done = false;
	nrw.nrw_port = IPC_PORT_UNKNOWN;
	for (;;) {
		ipc_dispatch_poll(id);
		if (nrw.nrw_done)
			break;
		ipc_dispatch_wait(id);
	}

	/*
	 * XXX Free port and ipc_dispatch.
	 */

	return (nrw.nrw_port);
}

int
ns_register(const char *service, ipc_port_t port)
{
	const struct ipc_dispatch_handler *idh;
	struct ns_register_request nsreq;
	struct ns_response_wait nrw;
	struct ipc_dispatch *id;
	int error;

	id = ipc_dispatch_allocate(IPC_PORT_UNKNOWN, IPC_PORT_FLAG_DEFAULT);
	idh = ipc_dispatch_register(id, ns_register_response_handler, &nrw);

	nsreq.error = 0;
	memset(nsreq.service_name, 0, NS_SERVICE_NAME_LENGTH);
	strlcpy(nsreq.service_name, service, NS_SERVICE_NAME_LENGTH);
	nsreq.port = port;

	error = ipc_dispatch_send(id, idh, IPC_PORT_NS, NS_MESSAGE_REGISTER, IPC_PORT_RIGHT_SEND_ONCE, &nsreq, sizeof nsreq);
	if (error != 0)
		fatal("ipc_dispatch_send failed", error);

	nrw.nrw_done = false;
	nrw.nrw_port = IPC_PORT_UNKNOWN;
	for (;;) {
		ipc_dispatch_poll(id);
		if (nrw.nrw_done)
			break;
		ipc_dispatch_wait(id);
	}

	/*
	 * XXX Free port and ipc_dispatch.
	 */

	return (nrw.nrw_error);
}

static void
ns_lookup_response_handler(const struct ipc_dispatch *id, const struct ipc_dispatch_handler *idh, const struct ipc_header *ipch, void *page)
{
	struct ns_response_wait *nrw = idh->idh_softc;
	struct ns_lookup_response *nsresp;

	(void)id;

	if (ipch->ipchdr_msg != IPC_MSG_REPLY(NS_MESSAGE_LOOKUP) ||
	    ipch->ipchdr_recsize != sizeof *nsresp || ipch->ipchdr_reccnt != 1 ||
	    page == NULL) {
		printf("Received unexpected message:\n");
		ipc_message_print(ipch, page);
		return;
	}

	nrw->nrw_done = true;

	nsresp = page;
	if (nsresp->error == 0)
		nrw->nrw_port = nsresp->port;
	nrw->nrw_error = nsresp->error;
}

static void
ns_register_response_handler(const struct ipc_dispatch *id, const struct ipc_dispatch_handler *idh, const struct ipc_header *ipch, void *page)
{
	struct ns_response_wait *nrw = idh->idh_softc;
	struct ns_register_response *nsresp;

	(void)id;

	if (ipch->ipchdr_msg != IPC_MSG_REPLY(NS_MESSAGE_REGISTER) ||
	    ipch->ipchdr_recsize != sizeof *nsresp || ipch->ipchdr_reccnt != 1 ||
	    page == NULL) {
		printf("Received unexpected message:\n");
		ipc_message_print(ipch, page);
		return;
	}

	nrw->nrw_done = true;

	nsresp = page;
	if (nsresp->error == 0)
		nrw->nrw_port = nsresp->port;
	nrw->nrw_error = nsresp->error;
}
