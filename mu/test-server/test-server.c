#include <core/types.h>
#include <core/error.h>
#include <core/printf.h>
#include <core/string.h>
#include <ipc/ipc.h>
#include <ns/ns.h>
#include <vm/vm_page.h>

#include <common/common.h>

void
main(void)
{
	struct ns_register_request *nsreq;
	struct ipc_header ipch;
	ipc_port_t port;
	void *page;
	int error;

	puts("Starting test-server.\n");

	error = vm_page_get(&page);
	if (error != 0)
		fatal("vm_page_get failed", error);

	error = ipc_port_allocate(&port, IPC_PORT_FLAG_DEFAULT);
	if (error != 0)
		fatal("ipc_port_allocate failed", error);

	ipch.ipchdr_src = port;
	ipch.ipchdr_dst = IPC_PORT_NS;
	ipch.ipchdr_msg = NS_MESSAGE_REGISTER;
	ipch.ipchdr_right = IPC_PORT_RIGHT_SEND_ONCE;
	ipch.ipchdr_cookie = 0;
	ipch.ipchdr_recsize = sizeof *nsreq;
	ipch.ipchdr_reccnt = 1;

	nsreq = page;
	nsreq->error = 0;
	memset(nsreq->service_name, 0, NS_SERVICE_NAME_LENGTH);
	strlcpy(nsreq->service_name, "test-server", NS_SERVICE_NAME_LENGTH);
	nsreq->port = port;

	printf("Sending message:\n");
	ipc_message_print(&ipch, page);

	error = ipc_port_send(&ipch, nsreq);
	if (error != 0)
		fatal("ipc_port_send failed", error);

	for (;;) {
		puts("Waiting.\n");
		error = ipc_port_wait(port);
		if (error != 0)
			fatal("ipc_port_wait failed", error);

		error = ipc_port_receive(port, &ipch, &page);
		if (error != 0)
			fatal("ipc_port_receive failed", error);

		if (page == NULL)
			fatal("got NULL page from ipc_port_receive", ERROR_UNEXPECTED);

		printf("Received message:\n");
		ipc_message_print(&ipch, page);

		/* XXX free page.  */
	}
}
