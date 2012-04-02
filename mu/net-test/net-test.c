#include <core/types.h>
#include <core/error.h>
#include <core/printf.h>
#include <core/string.h>
#include <io/network/interface.h>
#include <ipc/ipc.h>
#include <ns/ns.h>
#include <vm/vm_page.h>

#include <common/common.h>

struct if_context {
	const struct ipc_dispatch_handler *ifc_handler;
	ipc_port_t ifc_ifport;
};

static void if_transmit(struct if_context *, const void *, size_t);
static void if_response_handler(const struct ipc_dispatch_handler *, const struct ipc_header *, void *);

void
main(void)
{
	struct ipc_dispatch *id;
	struct if_context ifc;

	puts("Starting net-test.\n");

	/*
	 * Wait for the network interface.
	 */
	while ((ifc.ifc_ifport = ns_lookup("tmether0")) == IPC_PORT_UNKNOWN)
		continue;

	id = ipc_dispatch_allocate(IPC_PORT_UNKNOWN, IPC_PORT_FLAG_DEFAULT);
	ifc.ifc_handler = ipc_dispatch_register(id, if_response_handler, &ifc);

	if_transmit(&ifc, "123456!@#$%^\1\1___,___,___,___,1234123412341234", 64);

	ipc_dispatch(id);
}

static void
if_transmit(struct if_context *ifc, const void *data, size_t datalen)
{
	int error;

	error = ipc_dispatch_send(ifc->ifc_handler, ifc->ifc_ifport, NETWORK_INTERFACE_MSG_TRANSMIT, IPC_PORT_RIGHT_SEND_ONCE, data, datalen);
	if (error != 0)
		fatal("ipc_dispatch_send failed", error);
}

static void
if_response_handler(const struct ipc_dispatch_handler *idh, const struct ipc_header *ipch, void *page)
{
	int *errorp;

	(void)idh;

	if (ipch->ipchdr_msg != IPC_MSG_REPLY(NETWORK_INTERFACE_MSG_TRANSMIT) ||
	    ipch->ipchdr_recsize != sizeof *errorp || ipch->ipchdr_reccnt != 1 ||
	    page == NULL) {
		printf("Received unexpected message:\n");
		ipc_message_print(ipch, page);
		return;
	}

	errorp = page;
	if (*errorp != 0) {
		printf("Error from interface transmit: %m\n", *errorp);
		ipc_message_print(ipch, page);
		return;
	}
}
