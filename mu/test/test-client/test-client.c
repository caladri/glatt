#include <core/types.h>
#include <core/error.h>
#include <core/printf.h>
#include <core/string.h>
#include <ipc/ipc.h>
#include <ns/ns.h>
#include <vm/vm_page.h>

#include <libmu/common.h>
#include <libmu/ipc_request.h>

void
main(void)
{
	struct ipc_request_message req;
	struct ipc_response_message resp;
	ipc_port_t server;
	int error;

	puts("Starting test-client.\n");

	/*
	 * Wait for the test-server.
	 */
	while ((server = ns_lookup("test-server")) == IPC_PORT_UNKNOWN)
		continue;

	memset(&req, 0, sizeof req);
	memset(&resp, 0, sizeof resp);

	req.src = IPC_PORT_UNKNOWN;
	req.dst = server;
	req.msg = 1;
	req.param = 0;

	error = ipc_request(&req, &resp);
	if (error != 0)
		fatal("ipc_request failed", error);

	if (resp.error != 0)
		fatal("error from test-server", resp.error);

	fatal("Finished!", 0);
}
