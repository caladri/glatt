#include <core/types.h>
#include <core/error.h>
#include <core/printf.h>
#include <core/string.h>
#include <ipc/ipc.h>
#include <ns/ns.h>
#include <vm/vm_page.h>

#include <common/common.h>

static void test_request(const struct ipc_dispatch_handler *, ipc_port_t server);
static void test_response_handler(const struct ipc_dispatch_handler *, const struct ipc_header *, void *);

void
main(void)
{
	const struct ipc_dispatch_handler *idh;
	struct ipc_dispatch *id;
	ipc_port_t server;

	puts("Starting test-client.\n");

	/*
	 * Wait for the test-server.
	 */
	while ((server = ns_lookup("test-server")) == IPC_PORT_UNKNOWN)
		continue;

	id = ipc_dispatch_alloc(IPC_PORT_UNKNOWN);

	idh = ipc_dispatch_register(id, test_response_handler, NULL);
	test_request(idh, server);

	ipc_dispatch(id);
}

static void
test_request(const struct ipc_dispatch_handler *idh, ipc_port_t server)
{
	int error;

	error = ipc_dispatch_send(idh, server, 1, IPC_PORT_RIGHT_SEND_ONCE, NULL, 0);
	if (error != 0)
		fatal("ipc_dispatch_send failed", error);
}

static void
test_response_handler(const struct ipc_dispatch_handler *idh, const struct ipc_header *ipch, void *page)
{
	if (ipch->ipchdr_msg != IPC_MSG_REPLY(1) ||
	    ipch->ipchdr_recsize != 0 || ipch->ipchdr_reccnt != 0 ||
	    page != NULL) {
		printf("Received unexpected message:\n");
		ipc_message_print(ipch, page);
		return;
	}

	printf("Reply from test-server:\n");
	ipc_message_print(ipch, page);

	test_request(idh, ipch->ipchdr_src);
}