#include <core/types.h>
#include <core/error.h>
#include <core/printf.h>
#include <core/string.h>
#include <ipc/ipc.h>
#include <ns/ns.h>
#include <vm/vm_page.h>

#include <common/common.h>

static void ns_request(const struct ipc_dispatch_handler *);
static void ns_response_handler(const struct ipc_dispatch_handler *, const struct ipc_header *, void *);

void
main(void)
{
	const struct ipc_dispatch_handler *idh;
	struct ipc_dispatch *id;

	puts("Starting test-client.\n");

	id = ipc_dispatch_alloc(IPC_PORT_UNKNOWN);

	idh = ipc_dispatch_register(id, ns_response_handler, NULL);
	ns_request(idh);

	ipc_dispatch(id);
}

static void
ns_request(const struct ipc_dispatch_handler *idh)
{
	struct ns_lookup_request nsreq;
	int error;

	nsreq.error = 0;
	memset(nsreq.service_name, 0, NS_SERVICE_NAME_LENGTH);
	strlcpy(nsreq.service_name, "test-server", NS_SERVICE_NAME_LENGTH);

	error = ipc_dispatch_send(idh, IPC_PORT_NS, NS_MESSAGE_LOOKUP, IPC_PORT_RIGHT_SEND_ONCE, &nsreq, sizeof nsreq);
	if (error != 0)
		fatal("ipc_dispatch_send failed", error);
}

static void
ns_response_handler(const struct ipc_dispatch_handler *idh, const struct ipc_header *ipch, void *page)
{
	struct ns_lookup_response *nsresp;

	if (ipch->ipchdr_msg != IPC_MSG_REPLY(NS_MESSAGE_LOOKUP) ||
	    ipch->ipchdr_recsize != sizeof *nsresp || ipch->ipchdr_reccnt != 1 ||
	    page == NULL) {
		printf("Received unexpected message:\n");
		ipc_message_print(ipch, page);
		return;
	}

	nsresp = page;
	if (nsresp->error != 0) {
		/*
		 * Continue to wait for test-server to arrive.
		 */
		ns_request(idh);
		return;
	}

	printf("Discovered test-server:\n");
	ipc_message_print(ipch, page);
}
