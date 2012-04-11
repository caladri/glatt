#include <core/types.h>
#include <core/error.h>
#include <core/printf.h>
#include <core/string.h>
#include <ipc/ipc.h>
#include <ns/ns.h>
#include <vm/vm_page.h>

#include <common/common.h>

static void test_request_handler(const struct ipc_dispatch *id, const struct ipc_dispatch_handler *, const struct ipc_header *, void *);

void
main(void)
{
	struct ipc_dispatch *id;
	int error;

	puts("Starting test-server.\n");

	id = ipc_dispatch_allocate(IPC_PORT_UNKNOWN, IPC_PORT_FLAG_DEFAULT | IPC_PORT_FLAG_PUBLIC);

	error = ns_register("test-server", id->id_port);
	if (error != 0)
		fatal("ns_register failed", error);

	ipc_dispatch_register_default(id, test_request_handler, NULL);

	ipc_dispatch(id);

	ipc_dispatch_free(id);
}

static void
test_request_handler(const struct ipc_dispatch *id, const struct ipc_dispatch_handler *idh, const struct ipc_header *reqh, void *page)
{
	int error;

	(void)idh;

	if (reqh->ipchdr_recsize != 0 || reqh->ipchdr_reccnt != 0 ||
	    page != NULL) {
		ipc_message_drop(reqh, page);
		return;
	}

	error = ipc_dispatch_send_reply(id, reqh, IPC_PORT_RIGHT_NONE, NULL, 0);
	if (error != 0) {
		printf("Failed to send reply to:\n");
		ipc_message_print(reqh, page);
	}
}
