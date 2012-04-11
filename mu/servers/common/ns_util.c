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

	ipc_dispatch_free(id);

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

	ipc_dispatch_free(id);

	return (nrw.nrw_error);
}

static void
ns_lookup_response_handler(const struct ipc_dispatch *id, const struct ipc_dispatch_handler *idh, const struct ipc_header *ipch, void *page)
{
	struct ns_response_wait *nrw = idh->idh_softc;
	struct ns_lookup_response *nsresp;
	struct ipc_error_record *err;

	(void)id;

	switch (ipch->ipchdr_msg) {
	case IPC_MSG_REPLY(NS_MESSAGE_LOOKUP):
		if (ipch->ipchdr_recsize != sizeof *nsresp || ipch->ipchdr_reccnt != 1 || page == NULL) {
			ipc_message_drop(ipch, page);
			return;
		}
		nsresp = page;

		nrw->nrw_done = true;
		nrw->nrw_port = nsresp->port;
		return;
	case IPC_MSG_ERROR(NS_MESSAGE_LOOKUP):
		if (ipch->ipchdr_recsize != sizeof *err || ipch->ipchdr_reccnt != 1 || page == NULL) {
			ipc_message_drop(ipch, page);
			return;
		}
		err = page;

		nrw->nrw_done = true;
		nrw->nrw_error = err->error;
		return;
	default:
		ipc_message_drop(ipch, page);
		return;
	}
}

static void
ns_register_response_handler(const struct ipc_dispatch *id, const struct ipc_dispatch_handler *idh, const struct ipc_header *ipch, void *page)
{
	struct ns_response_wait *nrw = idh->idh_softc;
	struct ipc_error_record *err;

	(void)id;

	switch (ipch->ipchdr_msg) {
	case IPC_MSG_REPLY(NS_MESSAGE_REGISTER):
		if (ipch->ipchdr_recsize != 0 || ipch->ipchdr_reccnt != 0 || page != NULL) {
			ipc_message_drop(ipch, page);
			return;
		}

		nrw->nrw_done = true;
		return;
	case IPC_MSG_ERROR(NS_MESSAGE_REGISTER):
		if (ipch->ipchdr_recsize != sizeof *err || ipch->ipchdr_reccnt != 1 || page == NULL) {
			ipc_message_drop(ipch, page);
			return;
		}
		err = page;

		nrw->nrw_done = true;
		nrw->nrw_error = err->error;
		return;
	default:
		ipc_message_drop(ipch, page);
		return;
	}
}
